#ifndef _INPUT_BOOSTER_H_#define _INPUT_BOOSTER_H_#include <linux/cpufreq.h>
enum input_booster_id {	INPUT_BOOSTER_ID_TSP = 0,	INPUT_BOOSTER_ID_TKEY,	INPUT_BOOSTER_ID_WACOM,};
#define DVFS_STAGE_NONE		1 << 0	// 0000 0000 0000 0001
#define DVFS_STAGE_SINGLE	1 << 1	// 0000 0000 0000 0010
#define DVFS_STAGE_DUAL		1 << 2	// 0000 0000 0000 0100
#define DVFS_STAGE_TRIPLE	1 << 3	// 0000 0000 0000 1000
#define DVFS_STAGE_PENTA	1 << 5	// 0000 0000 0010 0000
#define DVFS_STAGE_NINTH	1 << 9	// 0000 0010 0000 0000
#define DVFS_TSP_STAGE		(DVFS_STAGE_NONE | DVFS_STAGE_SINGLE |\				DVFS_STAGE_DUAL)#define DVFS_TKEY_STAGE		(DVFS_STAGE_NONE | DVFS_STAGE_SINGLE |\				DVFS_STAGE_DUAL)#define DVFS_WACOM_STAGE		(DVFS_STAGE_NONE | DVFS_STAGE_SINGLE |\				DVFS_STAGE_DUAL)#ifndef DVFS_TSP_STAGE#define DVFS_TSP_STAGE		0#endif#ifndef DVFS_TKEY_STAGE#define DVFS_TKEY_STAGE	0#endif#ifndef DVFS_WACOM_STAGE#define DVFS_WACOM_STAGE	0#endif#if DVFS_TSP_STAGE#define COMMON_INPUT_BOOSTER#endif
/* TSP */
#define INPUT_BOOSTER_OFF_TIME_TSP		500
#define INPUT_BOOSTER_CHG_TIME_TSP		130
#define INPUT_BOOSTER_HIGH_OFF_TIME_TSP		1000
#define INPUT_BOOSTER_HIGH_CHG_TIME_TSP		500

/* Touchkey */
#define INPUT_BOOSTER_OFF_TIME_TKEY		500
#define INPUT_BOOSTER_CHG_TIME_TKEY		500

/* Wacom */
#define INPUT_BOOSTER_OFF_TIME_WACOM		500
#define INPUT_BOOSTER_CHG_TIME_WACOM		130

struct input_booster {
	struct delayed_work	work_dvfs_off;
	struct delayed_work	work_dvfs_chg;
	struct mutex		dvfs_lock;
	bool dvfs_lock_status;
	int dvfs_old_stauts;
	int dvfs_boost_mode;
	int dvfs_freq;
	int dvfs_id;
	int dvfs_stage;
	int (*dvfs_off)(struct input_booster *);
	void (*dvfs_set)(struct input_booster *, int);
};
void input_booster_init_dvfs(struct input_booster *booster, int id);#endif /* _INPUT_BOOSTER_H_ */
