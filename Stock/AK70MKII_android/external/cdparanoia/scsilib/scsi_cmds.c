#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <memory.h>
#include <schily.h>

#include "scsilib.h"

//#define	strbeg(s1,s2)	(strstr((s2), (s1)) == (s2))

static int strbeg( const char* s1, const char* s2)
{
  return ( ((char*)strstr(s2,s1)) == s2 );
}

EXPORT int inquiry(SCSI* scgp,caddr_t bp,int cnt)
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	memset(bp,0, cnt);
	memset(scmd,0,sizeof(*scmd));
	scmd->addr = bp;
	scmd->size = cnt;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->target = scgp->target;
	scmd->cdb.g0_cdb.cmd = SC_INQUIRY;
	scmd->cdb.g0_cdb.lun = scgp->lun;
	scmd->cdb.g0_cdb.count = cnt;
	
	scgp->cmdname = "inquiry";

	if (scsicmd(scgp) < 0)
		return (-1);
	if (scgp->verbose)
		scsiprbytes("Inquiry Data   :", (u_char *)bp, cnt - scsigetresid(scgp));
	return (0);
}


EXPORT BOOL unit_ready(SCSI	*scgp)
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	if (test_unit_ready(scgp) >= 0)		/* alles OK */
		return (TRUE);
	else if (scmd->error >= SCG_FATAL)	/* nicht selektierbar */
		return (FALSE);

	if (scmd->sense.code < 0x70) {		/* non extended Sense */
		if (scmd->sense.code == 0x04)	/* NOT_READY */
			return (FALSE);
		return (TRUE);
	}
	if (((struct scsi_ext_sense *)&scmd->sense)->key == SC_UNIT_ATTENTION) {
		if (test_unit_ready(scgp) >= 0)	/* alles OK */
			return (TRUE);
	}
						/* FALSE wenn NOT_READY */
	return (((struct scsi_ext_sense *)&scmd->sense)->key != SC_NOT_READY);
}

EXPORT BOOL wait_unit_ready(SCSI* scgp,int secs)
{
	int	i;
	int	c;
	int	k;
	int	ret;

	scgp->silent++;
	ret = test_unit_ready(scgp);		/* eat up unit attention */
	scgp->silent--;

	if (ret >= 0)				/* success that's enough */
		return (TRUE);

	scgp->silent++;
	for (i=0; i < secs && (ret = test_unit_ready(scgp)) < 0; i++) {
		if (scgp->scmd->scb.busy != 0) {
		  usleep(1000000);
			continue;
		}
		c = scsi_sense_code(scgp);
		k = scsi_sense_key(scgp);
		if (k == SC_NOT_READY && (c == 0x3A || c == 0x30)) {
			scsiprinterr(scgp);
			scgp->silent--;
			return (FALSE);
		}
		usleep(1000000);
	}
	scgp->silent--;
	if (ret < 0)
		return (FALSE);
	return (TRUE);
}

EXPORT int test_unit_ready(SCSI	*scgp)
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	memset(scmd,0,sizeof(*scmd));
	scmd->addr = (caddr_t)0;
	scmd->size = 0;
	scmd->flags = SCG_DISRE_ENA | (scgp->silent ? SCG_SILENT:0);
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->target = scgp->target;
	scmd->cdb.g0_cdb.cmd = SC_TEST_UNIT_READY;
	scmd->cdb.g0_cdb.lun = scgp->lun;
	
	scgp->cmdname = "test unit ready";

	return (scsicmd(scgp));
}

EXPORT BOOL getdev(SCSI* scgp,BOOL print)
{
		 BOOL	got_inquiry = TRUE;
		 char	vendor_info[8+1];
		 char	prod_ident[16+1];
		 char	prod_revision[4+1];
	register struct	scg_cmd	*scmd = scgp->scmd;
	register struct scsi_inquiry *inq = scgp->inq;


	memset(inq,0, sizeof(*inq));
	scgp->dev = DEV_UNKNOWN;
	scgp->silent++;
	(void)unit_ready(scgp);
	if (scmd->error >= SCG_FATAL &&
				!(scmd->scb.chk && scmd->sense_count > 0)) {
		scgp->silent--;
		return (FALSE);
	}


/*	if (scmd->error < SCG_FATAL || scmd->scb.chk && scmd->sense_count > 0){*/

	if (inquiry(scgp, (caddr_t)inq, sizeof(*inq)) < 0) {
		got_inquiry = FALSE;
		if (scgp->verbose) {
			printf(
		"error: %d scb.chk: %d sense_count: %d sense.code: 0x%x\n",
				scmd->error, scmd->scb.chk,
				scmd->sense_count, scmd->sense.code);
		}
			/*
			 * Folgende Kontroller kennen das Kommando
			 * INQUIRY nicht:
			 *
			 * ADAPTEC	ACB-4000, ACB-4010, ACB 4070
			 * SYSGEN	SC4000
			 *
			 * Leider reagieren ACB40X0 und ACB5500 identisch
			 * wenn drive not ready (code == not ready),
			 * sie sind dann nicht zu unterscheiden.
			 */

		if (scmd->scb.chk && scmd->sense_count == 4) {
			/* Test auf SYSGEN				 */
			(void)qic02(scgp, 0x12);	/* soft lock on  */
			if (qic02(scgp, 1) < 0) {	/* soft lock off */
				scgp->dev = DEV_ACB40X0;
/*				scgp->dev = acbdev();*/
			} else {
				scgp->dev = DEV_SC4000;
				inq->type = INQ_SEQD;
				inq->removable = 1;
			}
		}
	} else if (scgp->verbose) {
		int	i;
		int	len = inq->add_len + 5;
		u_char	ibuf[256+5];
		u_char	*ip = (u_char *)inq;
		u_char	c;

		if (len > (int)sizeof (*inq) &&
				inquiry(scgp, (caddr_t)ibuf, inq->add_len+5) >= 0) {
			len = inq->add_len+5 - scsigetresid(scgp);
			ip = ibuf;
		} else {
			len = sizeof (*inq);
		}
		printf("Inquiry Data   : ");
		for (i = 0; i < len; i++) {
			c = ip[i];
			if (c >= ' ' && c < 0177)
				printf("%c", c);
			else
				printf(".");
		}
		printf("\n");
	}

	strncpy(vendor_info, inq->vendor_info, sizeof(inq->vendor_info));
	strncpy(prod_ident, inq->prod_ident, sizeof(inq->prod_ident));
	strncpy(prod_revision, inq->prod_revision, sizeof(inq->prod_revision));

	vendor_info[sizeof(inq->vendor_info)] = '\0';
	prod_ident[sizeof(inq->prod_ident)] = '\0';
	prod_revision[sizeof(inq->prod_revision)] = '\0';

	switch (inq->type) {

	case INQ_DASD:
		if (inq->add_len == 0) {
			if (scgp->dev == DEV_UNKNOWN && got_inquiry) {
				scgp->dev = DEV_ACB5500;
				strcpy(inq->vendor_info,
					"ADAPTEC ACB-5500        FAKE");

			} else switch (scgp->dev) {

			case DEV_ACB40X0:
				strcpy(inq->vendor_info,
					"ADAPTEC ACB-40X0        FAKE");
				break;
			case DEV_ACB4000:
				strcpy(inq->vendor_info,
					"ADAPTEC ACB-4000        FAKE");
				break;
			case DEV_ACB4010:
				strcpy(inq->vendor_info,
					"ADAPTEC ACB-4010        FAKE");
				break;
			case DEV_ACB4070:
				strcpy(inq->vendor_info,
					"ADAPTEC ACB-4070        FAKE");
				break;
			}
		} else if (inq->add_len < 31) {
			scgp->dev = DEV_NON_CCS_DSK;

		} else if (strbeg("EMULEX", vendor_info)) {
			if (strbeg("MD21", prod_ident))
				scgp->dev = DEV_MD21;
			if (strbeg("MD23", prod_ident))
				scgp->dev = DEV_MD23;
			else
				scgp->dev = DEV_CCS_GENDISK;
		} else if (strbeg("ADAPTEC", vendor_info)) {
			if (strbeg("ACB-4520", prod_ident))
				scgp->dev = DEV_ACB4520A;
			if (strbeg("ACB-4525", prod_ident))
				scgp->dev = DEV_ACB4525;
			else
				scgp->dev = DEV_CCS_GENDISK;
		} else if (strbeg("SONY", vendor_info) &&
					strbeg("SMO-C501", prod_ident)) {
			scgp->dev = DEV_SONY_SMO;
		} else {
			scgp->dev = DEV_CCS_GENDISK;
		}
		break;

	case INQ_SEQD:
		if (scgp->dev == DEV_SC4000) {
			strcpy(inq->vendor_info,
				"SYSGEN  SC4000          FAKE");
		} else if (inq->add_len == 0 &&
					inq->removable &&
						inq->ansi_version == 1) {
			scgp->dev = DEV_MT02;
			strcpy(inq->vendor_info,
				"EMULEX  MT02            FAKE");
		}
		break;

/*	case INQ_OPTD:*/
	case INQ_ROMD:
	case INQ_WORM:
		if (strbeg("RXT-800S", prod_ident))
			scgp->dev = DEV_RXT800S;

		/*
		 * Start of CD-Recorders:
		 */
		if (strbeg("ACER", vendor_info)) { 
			if (strbeg("CR-4020C", prod_ident)) 
                                scgp->dev = DEV_RICOH_RO_1420C;

		} else if (strbeg("CREATIVE", vendor_info)) { 
			if (strbeg("CDR2000", prod_ident))
				scgp->dev = DEV_RICOH_RO_1060C;

		} else if (strbeg("GRUNDIG", vendor_info)) { 
			if (strbeg("CDR100IPW", prod_ident))
				scgp->dev = DEV_CDD_2000;

		} else if (strbeg("JVC", vendor_info)) {
			if (strbeg("XR-W2001", prod_ident))
				scgp->dev = DEV_TEAC_CD_R50S;
			else if (strbeg("XR-W2010", prod_ident))
				scgp->dev = DEV_TEAC_CD_R50S;
			else if (strbeg("R2626", prod_ident))
				scgp->dev = DEV_TEAC_CD_R50S;

		} else if (strbeg("MITSBISH", vendor_info)) {

#ifdef	XXXX_REALLY
			/* It's MMC compliant */
			if (strbeg("CDRW226", prod_ident))
				scgp->dev = DEV_MMC_CDRW;
#endif

		} else if (strbeg("MITSUMI", vendor_info)) {
			/* Don't know any product string */
			scgp->dev = DEV_CDD_522;

		} else if (strbeg("PHILIPS", vendor_info) ||
				strbeg("IMS", vendor_info) ||
				strbeg("KODAK", vendor_info) ||
				strbeg("HP", vendor_info)) {

			if (strbeg("CDD521/00", prod_ident))
				scgp->dev = DEV_CDD_521_OLD;
			else if (strbeg("CDD521/02", prod_ident))
				scgp->dev = DEV_CDD_521_OLD;		/* PCD 200R? */
			else if (strbeg("CDD521", prod_ident))
				scgp->dev = DEV_CDD_521;

			if (strbeg("CDD522", prod_ident))
				scgp->dev = DEV_CDD_522;
			if (strbeg("PCD225", prod_ident))
				scgp->dev = DEV_CDD_522;
			if (strbeg("KHSW/OB", prod_ident))	/* PCD600 */
				scgp->dev = DEV_PCD_600;
			if (strbeg("CDR-240", prod_ident))
				scgp->dev = DEV_CDD_2000;

			if (strbeg("CDD20", prod_ident))
				scgp->dev = DEV_CDD_2000;
			if (strbeg("CDD26", prod_ident))
				scgp->dev = DEV_CDD_2600;

			if (strbeg("C4324/C4325", prod_ident))
				scgp->dev = DEV_CDD_2000;
			if (strbeg("CD-Writer 6020", prod_ident))
				scgp->dev = DEV_CDD_2600;

		} else if (strbeg("PINNACLE", vendor_info)) {
			if (strbeg("RCD-1000", prod_ident))
				scgp->dev = DEV_TEAC_CD_R50S;
			if (strbeg("RCD5020", prod_ident))
				scgp->dev = DEV_TEAC_CD_R50S;
			if (strbeg("RCD5040", prod_ident))
				scgp->dev = DEV_TEAC_CD_R50S;
			if (strbeg("RCD 4X4", prod_ident))
				scgp->dev = DEV_TEAC_CD_R50S;

		} else if (strbeg("PIONEER", vendor_info)) {
			if (strbeg("CD-WO DW-S114X", prod_ident))
				scgp->dev = DEV_PIONEER_DW_S114X;
			else if (strbeg("DVD-R DVR-S101", prod_ident))
				scgp->dev = DEV_PIONEER_DVDR_S101;

		} else if (strbeg("PLASMON", vendor_info)) {
			if (strbeg("RF4100", prod_ident))
				scgp->dev = DEV_PLASMON_RF_4100;
			else if (strbeg("CDR4220", prod_ident))
				scgp->dev = DEV_CDD_2000;

		} else if (strbeg("PLEXTOR", vendor_info)) {
			if (strbeg("CD-R   PX-R24CS", prod_ident))
				scgp->dev = DEV_RICOH_RO_1420C;

		} else if (strbeg("RICOH", vendor_info)) {
			if (strbeg("RO-1420C", prod_ident))
				scgp->dev = DEV_RICOH_RO_1420C;
			if (strbeg("RO1060C", prod_ident))
				scgp->dev = DEV_RICOH_RO_1060C;

		} else if (strbeg("SAF", vendor_info)) {	/* Smart & Friendly */
			if (strbeg("CD-R2004", prod_ident) ||
			    strbeg("CD-R2006 ", prod_ident))
				scgp->dev = DEV_SONY_CDU_924;
			else if (strbeg("CD-R2006PLUS", prod_ident))
				scgp->dev = DEV_TEAC_CD_R50S;
			else if (strbeg("CD-RW226", prod_ident))
				scgp->dev = DEV_TEAC_CD_R50S;
			else if (strbeg("CD-R4012", prod_ident))
				scgp->dev = DEV_TEAC_CD_R50S;

		} else if (strbeg("SONY", vendor_info)) {
			if (strbeg("CD-R   CDU92", prod_ident) ||
			    strbeg("CD-R   CDU94", prod_ident))
				scgp->dev = DEV_SONY_CDU_924;

		} else if (strbeg("TEAC", vendor_info)) {
			if (strbeg("CD-R50S", prod_ident) ||
			    strbeg("CD-R55S", prod_ident))
				scgp->dev = DEV_TEAC_CD_R50S;

		} else if (strbeg("TRAXDATA", vendor_info) ||
				strbeg("Traxdata", vendor_info)) {
			if (strbeg("CDR4120", prod_ident))
				scgp->dev = DEV_TEAC_CD_R50S;

		} else if (strbeg("T.YUDEN", vendor_info)) {
			if (strbeg("CD-WO EW-50", prod_ident))
				scgp->dev = DEV_CDD_521;

		} else if (strbeg("WPI", vendor_info)) {	/* Wearnes */
			if (strbeg("CDR-632P", prod_ident))
				scgp->dev = DEV_CDD_2600;

		} else if (strbeg("YAMAHA", vendor_info)) {
			if (strbeg("CDR10", prod_ident))
				scgp->dev = DEV_YAMAHA_CDR_100;
			if (strbeg("CDR200", prod_ident))
				scgp->dev = DEV_YAMAHA_CDR_400;
			if (strbeg("CDR400", prod_ident))
				scgp->dev = DEV_YAMAHA_CDR_400;

		} else if (strbeg("MATSHITA", vendor_info)) {
			if (strbeg("CD-R   CW-7501", prod_ident))
				scgp->dev = DEV_MATSUSHITA_7501;
			if (strbeg("CD-R   CW-7502", prod_ident))
				scgp->dev = DEV_MATSUSHITA_7502;
		}
		if (scgp->dev == DEV_UNKNOWN) {
			/*
			 * We do not have Manufacturer strings for
			 * the following drives.
			 */
			if (strbeg("CDS615E", prod_ident))	/* Olympus */
				scgp->dev = DEV_SONY_CDU_924;
		}
		if (scgp->dev == DEV_UNKNOWN && inq->type == INQ_ROMD) {
			BOOL	cdrr	 = FALSE;
			BOOL	cdwr	 = FALSE;
			BOOL	cdrrw	 = FALSE;
			BOOL	cdwrw	 = FALSE;
			BOOL	dvd	 = FALSE;

			scgp->dev = DEV_CDROM;

			if (mmc_check(scgp, &cdrr, &cdwr, &cdrrw, &cdwrw, &dvd))
				scgp->dev = DEV_MMC_CDROM;
			if (cdwr)
				scgp->dev = DEV_MMC_CDR;
			if (cdwrw)
				scgp->dev = DEV_MMC_CDRW;
			if (dvd)
				scgp->dev = DEV_MMC_DVD;
		}

	case INQ_PROCD:
		if (strbeg("BERTHOLD", vendor_info)) {
			if (strbeg("", prod_ident))
				scgp->dev = DEV_HRSCAN;
		}
		break;

	case INQ_SCAN:
		scgp->dev = DEV_MS300A;
		break;
	}
	scgp->silent--;
	if (!print)
		return (TRUE);

	if (scgp->dev == DEV_UNKNOWN && !got_inquiry) {
#ifdef	PRINT_INQ_ERR
		scsiprinterr(scgp);
#endif
		return (FALSE);
	}

	printf("Device type    : ");
	scsiprintdev(inq);
	printf("Version        : %d\n", inq->ansi_version);
	printf("Response Format: %d\n", inq->data_format);
	if (inq->data_format >= 2) {
		printf("Capabilities   : ");
		if (inq->aenc)		printf("AENC ");
		if (inq->termiop)	printf("TERMIOP ");
		if (inq->reladr)	printf("RELADR ");
		if (inq->wbus32)	printf("WBUS32 ");
		if (inq->wbus16)	printf("WBUS16 ");
		if (inq->sync)		printf("SYNC ");
		if (inq->linked)	printf("LINKED ");
		if (inq->cmdque)	printf("CMDQUE ");
		if (inq->softreset)	printf("SOFTRESET ");
		printf("\n");
	}
	if (inq->add_len >= 31 ||
			inq->info[0] || inq->ident[0] || inq->revision[0]) {
		printf("Vendor_info    : '%.8s'\n", inq->info);
		printf("Identifikation : '%.16s'\n", inq->ident);
		printf("Revision       : '%.4s'\n", inq->revision);
	}
	return (TRUE);
}


EXPORT BOOL is_cddrive(SCSI* scgp)
{
	return (scgp->inq->type == INQ_ROMD || scgp->inq->type == INQ_WORM);
}



EXPORT int qic02(SCSI* scgp,int cmd)
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	memset(scmd,0,sizeof(*scmd));
	scmd->addr = (caddr_t)0;
	scmd->size = 0;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = DEF_SENSE_LEN;
	scmd->target = scgp->target;
	scmd->cdb.g0_cdb.cmd = 0x0D;	/* qic02 Sysgen SC4000 */
	scmd->cdb.g0_cdb.lun = scgp->lun;
	scmd->cdb.g0_cdb.mid_addr = cmd;
	
	scgp->cmdname = "qic 02";
	return (scsicmd(scgp));
}


EXPORT BOOL is_mmc(SCSI* scgp,BOOL* dvdp)
{
	return (mmc_check(scgp, NULL, NULL, NULL, NULL, dvdp));
}


EXPORT BOOL mmc_check(SCSI* scgp,BOOL* cdrrp,BOOL* cdwrp,BOOL* cdrrwp,BOOL* cdwrwp,BOOL* dvdp)
{
	u_char	mode[0x100];
	BOOL	was_atapi;
	struct	cd_mode_page_2A *mp;

	if (scgp->inq->type != INQ_ROMD)
		return (FALSE);

	memset(mode,0,sizeof(mode));

	was_atapi = allow_atapi(scgp, TRUE);
	scgp->silent++;
	mp = mmc_cap(scgp, mode);
	scgp->silent--;
	allow_atapi(scgp, was_atapi);
	if (mp == NULL)
		return (FALSE);

	mmc_getval(mp, cdrrp, cdwrp, cdrrwp, cdwrwp, dvdp);

	return (TRUE);			/* Generic SCSI-3/mmc CD	*/
}


LOCAL BOOL	is_atapi;

EXPORT BOOL allow_atapi(SCSI* scgp,BOOL bNew)
{
	BOOL	old = is_atapi;
	u_char	mode[256];

	if (bNew == old)
		return (old);

	scgp->silent++;
	if (bNew &&
	    mode_sense_g1(scgp, mode, 8, 0x3F, 0) < 0) {	/* All pages current */
		bNew = FALSE;
	}
	scgp->silent--;

	is_atapi = bNew;
	return (old);
}


EXPORT struct cd_mode_page_2A * mmc_cap(SCSI* scgp,u_char* modep)
{
	int	len;
	int	val;
	u_char	mode[0x100];
	struct	cd_mode_page_2A *mp;
	struct	cd_mode_page_2A *mp2;


	memset(mode,0,sizeof(mode));

	if (!get_mode_params(scgp, 0x2A, "CD capabilities",
			mode, (u_char *)0, (u_char *)0, (u_char *)0, &len))
		return (NULL);

	if (len == 0)			/* Pre SCSI-3/mmc drive	 	*/
		return (NULL);

	mp = (struct cd_mode_page_2A *)
		(mode + sizeof(struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);

	/*
	 * Do some heuristics against pre SCSI-3/mmc VU page 2A
	 * We should test for a minimum p_len of 0x14, but some
	 * buggy CD-ROM readers ommit the write speed values.
	 */
	if (mp->p_len < 0x10)
		return (NULL);

	val = a_to_u_2_byte(mp->max_read_speed);
	if (val != 0 && val < 176)
		return (NULL);

	val = a_to_u_2_byte(mp->cur_read_speed);
	if (val != 0 && val < 176)
		return (NULL);

	len -= sizeof(struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len;
	if (modep)
		mp2 = (struct cd_mode_page_2A *)modep;
	else
		mp2 = (struct cd_mode_page_2A *)malloc(len);
	if (mp2)
		memcpy(mp2,mp,len);

	return (mp2);
}


EXPORT void mmc_getval(
						struct	cd_mode_page_2A *mp,
						BOOL*	cdrrp,
						BOOL*	cdwrp,
						BOOL*	cdrrwp,
						BOOL*	cdwrwp,
						BOOL*	dvdp)
{
	if (cdrrp)
		*cdrrp = (mp->cd_r_read != 0);	/* SCSI-3/mmc CD	*/
	if (cdwrp)
		*cdwrp = (mp->cd_r_write != 0);	/* SCSI-3/mmc CD-R	*/

	if (cdrrwp)
		*cdrrwp = (mp->cd_rw_read != 0);/* SCSI-3/mmc CD	*/
	if (cdwrwp)
		*cdwrwp = (mp->cd_rw_write != 0);/* SCSI-3/mmc CD-RW	*/
	if (dvdp) {
		*dvdp = 			/* SCSI-3/mmc2 DVD 	*/
		(mp->dvd_ram_read + mp->dvd_r_read  + mp->dvd_rom_read +
		 mp->dvd_ram_write + mp->dvd_r_write) != 0;
	}
}


EXPORT int mode_sense_g1(SCSI* scgp,u_char* dp,int	cnt,int	page,int pcf)
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	memset(scmd,0,sizeof(*scmd));
	scmd->addr = (caddr_t)dp;
	scmd->size = cnt;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->target = scgp->target;
	scmd->cdb.g1_cdb.cmd = 0x5A;
	scmd->cdb.g1_cdb.lun = scgp->lun;
#ifdef	nonono
	scmd->cdb.g0_cdb.high_addr = 1<<4;	/* DBD Disable Block desc. */
#endif
	scmd->cdb.g1_cdb.addr[0] = (page&0x3F) | ((pcf<<6)&0xC0);
	g1_cdblen(&scmd->cdb.g1_cdb, cnt);

	scgp->cmdname = "mode sense g1";

	if (scsicmd(scgp) < 0)
		return (-1);
	if (scgp->verbose) scsiprbytes("Mode Sense Data", dp, cnt - scsigetresid(scgp));
	return (0);
}


EXPORT BOOL get_mode_params(
								SCSI	*scgp,
								int	page,
								char	*pagename,
								u_char	*modep,
								u_char	*cmodep,
								u_char	*dmodep,
								u_char	*smodep,
								int	*lenp)
{
	int	len;
	BOOL	ret = TRUE;

#ifdef	XXX
	if (lenp)
		*lenp = 0;
	if (!has_mode_page(scgp, page, pagename, &len)) {
		if (!scgp->silent) errmsgno(EX_BAD,
			"Warning: controller does not support %s page.\n",
								pagename);
		return (TRUE);	/* Hoffentlich klappt's trotzdem */
	}
	if (lenp)
		*lenp = len;
#else
	if (lenp == 0)
		len = 0xFF;
#endif

	if (modep)
	{
		memset(modep,0,0x100);
		if (mode_sense(scgp, modep, len, page, 0) < 0) {/* Page x current */
			printf("Cannot get %s data.\n", pagename);
			ret = FALSE;
		} else if (scgp->verbose)
		{
			scsiprbytes("Mode Sense Data", modep, len - scsigetresid(scgp));
		}
	}

	if (cmodep) {
		memset(cmodep,0,0x100);
		if (mode_sense(scgp, cmodep, len, page, 1) < 0)
		{	/* Page x change */
			printf("Cannot get %s mask.\n", pagename);
			ret = FALSE;
		} else if (scgp->verbose)
		{
			scsiprbytes("Mode Sense Data", cmodep, len - scsigetresid(scgp));
		}
	}

	if (dmodep) {
		memset(dmodep,0,0x100);
		if (mode_sense(scgp, dmodep, len, page, 2) < 0) {/* Page x default */
			printf("Cannot get default %s data.\n",pagename);
			ret = FALSE;
		} else if (scgp->verbose)
		{
			scsiprbytes("Mode Sense Data", dmodep, len - scsigetresid(scgp));
		}
	}

	if (smodep) {
		memset(smodep,0,0x100);
		if (mode_sense(scgp, smodep, len, page, 3) < 0) {/* Page x saved */
			printf("Cannot get saved %s data.\n", pagename);
			ret = FALSE;
		} else if (scgp->verbose)
		{
			scsiprbytes("Mode Sense Data", smodep, len - scsigetresid(scgp));
		}
	}

	return (ret);
}


EXPORT int mode_sense(SCSI* scgp,u_char* dp,int	cnt,int	page,int pcf)
{
	if (is_atapi)
		return (mode_sense_sg0(scgp, dp, cnt, page, pcf));
	return (mode_sense_g0(scgp, dp, cnt, page, pcf));
}


EXPORT int mode_sense_sg0(SCSI* scgp,u_char* dp,int cnt,int page,int pcf)
{
	u_char	xmode[256+4];
	int		amt = cnt;
	int		len;

	if (amt < 1 || amt > 255) {
		/* XXX clear SCSI error codes ??? */
		return (-1);
	}

	memset(xmode,0,sizeof(xmode));
	if (amt < 4) {		/* Data length. medium type & VU */
		amt += 1;
	} else {
		amt += 4;
	}
	if (mode_sense_g1(scgp, xmode, amt, page, pcf) < 0)
		return (-1);

	amt = cnt - scsigetresid(scgp);
	if (amt > 4)
		memcpy(&dp[4],&xmode[8],amt-4);
	len = a_to_u_2_byte(xmode);
	if (len == 0) {
		dp[0] = 0;
	} else if (len < 6) {
		if (len > 2)
			len = 2;
		dp[0] = len;
	} else {
		dp[0] = len - 3;
	}
	dp[1] = xmode[2];
	dp[2] = xmode[3];
	len = a_to_u_2_byte(&xmode[6]);
	dp[3] = len;

	if (scgp->verbose) scsiprbytes("Mode Sense Data (converted)", dp, amt);
	return (0);
}

EXPORT int mode_sense_g0(SCSI* scgp,u_char* dp,int	cnt,int	page,int pcf)
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	memset(scmd,0,sizeof(*scmd));
	scmd->addr = (caddr_t)dp;
	scmd->size = 0xFF;
	scmd->size = cnt;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->target = scgp->target;
	scmd->cdb.g0_cdb.cmd = SC_MODE_SENSE;
	scmd->cdb.g0_cdb.lun = scgp->lun;
#ifdef	nonono
	scmd->cdb.g0_cdb.high_addr = 1<<4;	/* DBD Disable Block desc. */
#endif
	scmd->cdb.g0_cdb.mid_addr = (page&0x3F) | ((pcf<<6)&0xC0);
	scmd->cdb.g0_cdb.count = page ? 0xFF : 24;
	scmd->cdb.g0_cdb.count = cnt;

	scgp->cmdname = "mode sense g0";

	if (scsicmd(scgp) < 0)
		return (-1);
	if (scgp->verbose) scsiprbytes("Mode Sense Data", dp, cnt - scsigetresid(scgp));
	return (0);
}

EXPORT int read_subchannel(SCSI* scgp,caddr_t bp,int track,int cnt,int msf,int subq,int fmt)
{
	struct	scg_cmd	*scmd = scgp->scmd;

	memset(scmd,0,sizeof(*scmd));
	scmd->addr = bp;
	scmd->size = cnt;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->target = scgp->target;
	scmd->cdb.g1_cdb.cmd = 0x42;
	scmd->cdb.g1_cdb.lun = scgp->lun;
	if (msf)
		scmd->cdb.g1_cdb.res = 1;
	if (subq)
		scmd->cdb.g1_cdb.addr[0] = 0x40;
	scmd->cdb.g1_cdb.addr[1] = fmt;
	scmd->cdb.g1_cdb.res6 = track;
	g1_cdblen(&scmd->cdb.g1_cdb, cnt);

	scgp->cmdname = "read subchannel";

	if (scsicmd(scgp) < 0)
		return (-1);
	return (0);
}

EXPORT int read_toc(SCSI* scgp,caddr_t	bp,int track,int cnt,int msf,int fmt)
{
	struct	scg_cmd	*scmd = scgp->scmd;
	memset(scmd,0,sizeof(*scmd));
	scmd->addr = bp;
	scmd->size = cnt;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->target = scgp->target;
	scmd->cdb.g1_cdb.cmd = 0x43;
	scmd->cdb.g1_cdb.lun = scgp->lun;
	if (msf)
		scmd->cdb.g1_cdb.res = 1;
	scmd->cdb.g1_cdb.addr[0] = fmt & 0x0F;
	scmd->cdb.g1_cdb.res6 = track;
	g1_cdblen(&scmd->cdb.g1_cdb, cnt);

	scgp->cmdname = "read toc";

	if (scsicmd(scgp) < 0)
		return (-1);
	return (0);
}

EXPORT int read_toc_philips(SCSI* scgp,caddr_t bp,int track,int cnt,int msf,int fmt)
{
	struct	scg_cmd	*scmd = scgp->scmd;

	memset(scmd,0,sizeof(*scmd));
	scmd->addr = bp;
	scmd->size = cnt;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->target = scgp->target;
	scmd->timeout = 4 * 60;		/* May last  174s on a TEAC CD-R55S */
	scmd->cdb.g1_cdb.cmd = 0x43;
	scmd->cdb.g1_cdb.lun = scgp->lun;
	if (msf)
		scmd->cdb.g1_cdb.res = 1;
	scmd->cdb.g1_cdb.res6 = track;
	g1_cdblen(&scmd->cdb.g1_cdb, cnt);

	if (fmt & 1)
		scmd->cdb.g1_cdb.vu_96 = 1;
	if (fmt & 2)
		scmd->cdb.g1_cdb.vu_97 = 1;

	scgp->cmdname = "read toc";

	if (scsicmd(scgp) < 0)
		return (-1);
	return (0);
}

EXPORT int scsi_start_stop_unit(SCSI* scgp,int flg,int loej)
{
	struct	scg_cmd	*scmd = scgp->scmd;

	memset(scmd,0,sizeof(*scmd));
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->target = scgp->target;
	scmd->cdb.g0_cdb.cmd = 0x1B;	/* Start Stop Unit */
	scmd->cdb.g0_cdb.lun = scgp->lun;
	scmd->cdb.g0_cdb.count = (flg ? 1:0) | (loej ? 2:0);
	
	scgp->cmdname = "start/stop unit";

	return (scsicmd(scgp));
}

EXPORT int scsi_prevent_removal(SCSI* scgp,int prevent)
{
	struct	scg_cmd	*scmd = scgp->scmd;

	memset(scmd,0,sizeof(*scmd));
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->target = scgp->target;
	scmd->cdb.g0_cdb.cmd = 0x1E;
	scmd->cdb.g0_cdb.lun = scgp->lun;
	scmd->cdb.g0_cdb.count = prevent & 1;
	
	scgp->cmdname = "prevent/allow medium removal";

	if (scsicmd(scgp) < 0)
		return (-1);
	return (0);
}

EXPORT int scsi_set_speed(SCSI *scgp, int readspeed, int writespeed)
{
	register struct scg_cmd *scmd = scgp->scmd; 

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0'); 
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G5_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->target = scgp->target; 
	scmd->cdb.g5_cdb.cmd = 0xBB; 
	scmd->cdb.g5_cdb.lun = scgp->lun; 

	if (readspeed <= 0)
		i_to_2_byte(&scmd->cdb.g5_cdb.addr[0], 0xFFFF); 
	else
		i_to_2_byte(&scmd->cdb.g5_cdb.addr[0], readspeed); 

	if (writespeed <= 0)
		i_to_2_byte(&scmd->cdb.g5_cdb.addr[2], 0xFFFF); 
	else
		i_to_2_byte(&scmd->cdb.g5_cdb.addr[2], writespeed); 

	scgp->cmdname = "set cd speed";

	if (scsicmd(scgp) < 0)
		return (-1);
	return (0);
}

EXPORT int scsi_get_speed(SCSI *scgp, int *readspeedp, int *writespeedp)
{
	struct cd_mode_page_2A *mp; 
	Uchar m[256]; 
	int val; 
	
	scgp->silent++; 
	mp = mmc_cap(scgp, m);
	
	/* Get MMC capabilities in allocated mp */ 
	scgp->silent--; 
	
	if (mp == NULL) 
		return (-1); 
		
	/* Pre SCSI-3/mmc drive */ 
	val = a_to_u_2_byte(mp->cur_read_speed); 
	if (readspeedp) 
		*readspeedp = val; 
	
	val = a_to_u_2_byte(mp->cur_write_speed); 
	if (writespeedp) 
		*writespeedp = val; 
		
	return (0); 
}
