#ifndef SCSI_CMDS_INCLUDED
#define SCSI_CMDS_INCLUDED

#include <scg/scgcmd.h>
#include <scg/scsidefs.h>
#include <scg/scsireg.h>
#include <scg/scsitransp.h>



#define	FMT_TOC		0
#define	FMT_SINFO	1
#define	FMT_FULLTOC	2
#define	FMT_PMA		3
#define	FMT_ATIP	4
#define	FMT_CDTEXT	5

struct tocheader {
	u_char	len[2];
	u_char	first;
	u_char	last;
};

EXPORT BOOL wait_unit_ready(SCSI* scgp,int secs);
EXPORT int test_unit_ready(SCSI	*scgp);
EXPORT BOOL unit_ready(SCSI	*scgp);
EXPORT BOOL getdev(SCSI* scgp,BOOL print);
EXPORT BOOL is_cddrive(SCSI* scgp);
EXPORT int qic02(SCSI* scgp,int cmd);
EXPORT BOOL mmc_check(SCSI* scgp,BOOL* cdrrp,BOOL* cdwrp,BOOL* cdrrwp,BOOL* cdwrwp,BOOL* dvdp);
EXPORT BOOL is_mmc(SCSI* scgp,BOOL* dvdp);
EXPORT BOOL allow_atapi(SCSI* scgp,BOOL bNew);
EXPORT struct cd_mode_page_2A * mmc_cap(SCSI* scgp,u_char* modep);
EXPORT void mmc_getval( struct	cd_mode_page_2A *mp,BOOL* cdrrp, BOOL* cdwrp,
						BOOL* cdrrwp,BOOL* cdwrwp,BOOL* dvdp);
EXPORT BOOL get_mode_params(SCSI* scgp,int	page,char* pagename,u_char* modep,
								u_char* cmodep,u_char* dmodep,u_char* smodep,int* lenp);

EXPORT int mode_sense(SCSI* scgp,u_char* dp,int	cnt,int	page,int pcf);
EXPORT int mode_sense_sg0(SCSI* scgp,u_char* dp,int cnt,int page,int pcf);
EXPORT int mode_sense_g1(SCSI* scgp,u_char* dp,int	cnt,int	page,int pcf);
EXPORT int mode_sense_g0(SCSI* scgp,u_char* dp,int	cnt,int	page,int pcf);

EXPORT int read_toc(SCSI* scgp,caddr_t	bp,int track,int cnt,int msf,int fmt);
EXPORT int read_toc_philips(SCSI* scgp,caddr_t bp,int track,int cnt,int msf,int fmt);
EXPORT int read_subchannel(SCSI* scgp,caddr_t bp,int track,int cnt,int msf,int subq,int fmt);
EXPORT int scsi_start_stop_unit(SCSI* scgp,int flg,int loej);
EXPORT int scsi_prevent_removal(SCSI* scgp,int prevent);
EXPORT int scsi_set_speed(SCSI *scgp, int readspeed, int writespeed);
EXPORT int scsi_get_speed(SCSI *scgp, int *readspeedp, int *writespeedp);

#endif
