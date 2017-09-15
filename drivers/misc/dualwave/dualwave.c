/**
 * @file		dualwave.c
 * $Author: sirius.park@samsung.com $
 *			Communication Solution Lab. DM R&D Center,
 *			SAMSUNG ELECTRONICS CO., LTD.
 * $URL: $
 * $Revision: $
 *
 * @brief	if some file write operation is occured, the uevent will be delivered to user app which listen those information
 *			Time Write Location : /sys/kerenl/DualWave/sound/PLAY_TIME, /sys/kerenl/DualWave/sound/CAPTURE_TIME
 *			Time Format : %ld.%09ld (ex. 1.123123123, 1.100000000)
 *						note> if 1.1 is inserted, that value is understood to 1.000000001. so keep the format
 *			uevent Format : %ld.%09ld (ex. CAPTURE_TIME=2.000000001)
 *
 * @see

 * Copyright 2014 by Samsung Electronics, Inc.,
 *
 */
#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/kobject.h>
#include <linux/time.h>

#include <linux/jiffies.h>
#include <linux/timer.h>

#define MAX_DUALWAVE_MESSAGE_SIZE 1024
#define PRINT_TIMEFORMAT_STRING		"%ld.%09ld"
#define PRINT_CONFIG_FORMAT			"%d"
#define PRINT_CONFIG_FORMAT_STRING	"DUALWAVE_ENABLED="PRINT_CONFIG_FORMAT
#define PRINT_UEVNET_FORMAT_STRING	"%s="PRINT_TIMEFORMAT_STRING
#define SND_AVAIL_STRING			"SND_AVAIL=%d"


///////////////////////////////////////////////////////////////////////////////////////////////////
// Local Data Structures

struct dualwave_attribute{
		struct attribute attr;

		// operations
		ssize_t (*show) (struct dualwave_attribute *attr, char *buf);
		ssize_t (*store)(struct dualwave_attribute *attr, const char *buf, size_t count);

		// value
		struct timespec tTime;
		int tConfig;
};

// End of local  Data Structures
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
// Local Functions

// attributes related functions
#ifdef _DULAWAVE_TIME_INFO
static ssize_t show_my_attr_info (struct dualwave_attribute *attr, char *buf);
static ssize_t store_info_to_attr(struct dualwave_attribute *attr, const char *buf, size_t count);
#endif

static ssize_t show_my_config (struct dualwave_attribute *attr, char *buf);
static ssize_t store_my_config(struct dualwave_attribute *attr, const char *buf, size_t count);

// dualwave kobj sysfs operations
static ssize_t dualwave_attr_show(struct kobject *, struct attribute *,char *);
static ssize_t dualwave_attr_store(struct kobject *,struct attribute *,const char *, size_t);

// dualwave kobj release operations
static void dualwave_release(struct kobject *kobj);

// send time uevent to USER APP
int send_uevent_wh_timeinfo(const char *szName, struct timespec *ptTime);



static int err_status = 0;


// End of local function definitions
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
// glbal variables
#define DUALWAVE_INACTIVE	0
#define DUALWAVE_PLAYBACK	1
#define DUALWAVE_CAPTURE	2

//#define TIMER_VALUE		(10*HZ)
#define TIMER_VALUE		msecs_to_jiffies(10000)

struct timer_list expTimer;
void expTimer_service(unsigned long arg);

static struct kobject *g_ptDualWaveKobj = NULL;
static struct kset *g_ptDualWaveKset=NULL;
static int g_iEnableUpdateSoundTime = DUALWAVE_INACTIVE;
//static struct kset    *g_ptKset=NULL;

// the operations for kobject
static const struct sysfs_ops dualwave_sysfs_ops = {
		.show = dualwave_attr_show,
		.store = dualwave_attr_store,
};

// set private attribute information.
#if 0
static struct dualwave_attribute playtime_attribute =
		__ATTR(PLAY_TIME, 0664, show_my_attr_info, store_info_to_attr);
static struct dualwave_attribute capture_attribute =
		__ATTR(CAPTURE_TIME, 0664, show_my_attr_info, store_info_to_attr);
#endif
static struct dualwave_attribute config_attribute =
		__ATTR(DUALWAVE_CONFIG, 0664, show_my_config, store_my_config);

// the attr member of private attriubte structure is only registered to default_attrs
static struct attribute *dualwave_attrs[] = {
#if 0
		&playtime_attribute.attr,
		&capture_attribute.attr,
#endif
		&config_attribute.attr,
		NULL,									/* need to NULL terminate the list of attributes */
};


static struct kobj_type g_tKobjType = {
		.sysfs_ops = &dualwave_sysfs_ops,
		.release = dualwave_release,
		.default_attrs = dualwave_attrs,
};


void expTimer_service(unsigned long arg){
	g_iEnableUpdateSoundTime = DUALWAVE_INACTIVE;
	config_attribute.tConfig = 0;
	printk(KERN_INFO "khhan %s %d\n", __func__, __LINE__);
}

/**
  * @fn		ssize_t dualwave_attr_show(struct kobject *ptKobj, struct attribute *ptAttr,char *pBuf)
  * @brief	called whenever some file read operation in DualWave/sound is occured
  *
  * @param	kobject *ptKobj
  * @param	attribute *ptAttr
  * @param	*pBuf
  * @return	ssize_t
  * @warning
  */
ssize_t dualwave_attr_show(struct kobject *ptKobj, struct attribute *ptAttr,char *pBuf)
{
	struct dualwave_attribute *ptDualWaveAttr;

	//printk(KERN_INFO "khhan %s %d\n", __func__, __LINE__);

	// get dualwave structure pointer
	ptDualWaveAttr = container_of(ptAttr,struct dualwave_attribute , attr);

	if (!ptDualWaveAttr->show)
			return -EIO;

	// call the function of the operations linked to this attribute file
	return ptDualWaveAttr->show(ptDualWaveAttr, pBuf);

}

/**
  * @fn		ssize_t dualwave_attr_store(struct kobject *ptKobj,struct attribute *ptAttr,const char *pBuf, size_t nLen)
  * @brief	called whenever some file write operation in DualWave/sound is occured
  *
  * @param	kobject *ptKobj
  * @param	attribute *ptAttr
  * @param	char *pBuf
  * @param	nLen
  * @return	ssize_t
  * @warning
  */
ssize_t dualwave_attr_store(struct kobject *ptKobj,struct attribute *ptAttr,const char *pBuf, size_t nLen)
{
	struct dualwave_attribute *ptDualWaveAttr;

	//printk(KERN_INFO "khhan %s %d\n", __func__, __LINE__);

	// get dualwave structure pointer
	ptDualWaveAttr = container_of(ptAttr,struct dualwave_attribute , attr);

	if (!ptDualWaveAttr->store)
			return -EIO;

	// call the function of the operations linked to this attribute file
	return ptDualWaveAttr->store(ptDualWaveAttr, pBuf, nLen);

}

/**
  * @fn		static void dualwave_release(struct kobject *kobj)
  * @brief	called whenever some release operation in DualWave/sound is occured
  *
  * @param	kobject *kobj
  * @return	static
  * @warning
  */
static void dualwave_release(struct kobject *kobj)
{
	printk(KERN_INFO "khhan %s %d\n", __func__, __LINE__);

	kfree(kobj);
}

#if 0
/**
  * @fn		static ssize_t show_my_attr_info (struct dualwave_attribute *attr, char *buf)
  * @brief	just show current time information for debugging
  *
  * @param	dualwave_attribute *attr
  * @param	*buf
  * @return	static
  * @warning
  */
static ssize_t show_my_attr_info (struct dualwave_attribute *attr, char *buf)
{
	printk(KERN_INFO "khhan %s %d\n", __func__, __LINE__);

	return sprintf(buf, PRINT_TIMEFORMAT_STRING "\n", attr->tTime.tv_sec,attr->tTime.tv_nsec);
}
#endif

#if 0
/**
  * @fn		static ssize_t store_info_to_attr(struct dualwave_attribute *attr, const char *buf, size_t count)
  * @brief	send uevent to user appp with time information. the time informatin will be saved in module
  *
  * @param	dualwave_attribute *attr
  * @param	char *buf
  * @param	count
  * @return	ssize_t
  * @warning
  */
static ssize_t store_info_to_attr(struct dualwave_attribute *attr, const char *buf, size_t count)
{
	printk(KERN_INFO "khhan %s %d\n", __func__, __LINE__);

	sscanf(buf, PRINT_TIMEFORMAT_STRING, &attr->tTime.tv_sec, &attr->tTime.tv_nsec);
	send_uevent_wh_timeinfo(attr->attr.name, &attr->tTime);
	return count;
}
#endif

static ssize_t show_my_config (struct dualwave_attribute *attr, char *buf)
{
	//printk(KERN_INFO "khhan %s %d\n", __func__, __LINE__);

	//return sprintf(buf, PRINT_CONFIG_FORMAT "\n",g_iEnableUpdateSoundTime );
	return sprintf(buf, PRINT_CONFIG_FORMAT_STRING "\n", attr->tConfig);
}
static ssize_t store_my_config(struct dualwave_attribute *attr, const char *buf, size_t count)
{
	//printk(KERN_INFO "khhan %s %d\n", __func__, __LINE__);

	sscanf(buf, PRINT_CONFIG_FORMAT, &attr->tConfig);

	switch(attr->tConfig){
		case 0:
			attr->tConfig = 0;
			g_iEnableUpdateSoundTime = DUALWAVE_INACTIVE;
			//if(timer_pending(&expTimer)) {
			del_timer(&expTimer);
				printk(KERN_INFO "khhan del timer %s %d\n", __func__, __LINE__);
		break;
		case 1:
			attr->tConfig = 1;
			g_iEnableUpdateSoundTime = DUALWAVE_PLAYBACK;
			mod_timer(&expTimer, jiffies + TIMER_VALUE);
			printk(KERN_INFO "khhan modifying timer %s %d\n", __func__, __LINE__);
		break;
		case 2:
			attr->tConfig = 2;
			g_iEnableUpdateSoundTime = DUALWAVE_CAPTURE;
			mod_timer(&expTimer, jiffies + TIMER_VALUE);
			printk(KERN_INFO "khhan modifying timer %s %d\n", __func__, __LINE__);
		break;
		default:
			attr->tConfig = 0;
			g_iEnableUpdateSoundTime = DUALWAVE_INACTIVE;
			count = -1;
			printk(KERN_INFO "khhan ***** wrong status %s %d ***** \n", __func__, __LINE__);
	}

	return count;
}

int checkDualWaveStatus(void)
{
	return g_iEnableUpdateSoundTime;
}
EXPORT_SYMBOL(checkDualWaveStatus);

#if 0
void setDualWaveFunction(int bEnable)
{
	if (!bEnable)
	{
		g_iEnableUpdateSoundTime= DUALWAVE_FALSE;
	} else
	{
		g_iEnableUpdateSoundTime= DUALWAVE_TRUE;
	}

}
EXPORT_SYMBOL(setDualWaveFunction);
#endif

/**
  * @fn		static int send_uevent_wh_timeinfo(const char *szName, struct timespec *ptTime)
  * @brief	send the played or captured time info to user application via uevent \n
  *			string format is like as follows \n
  *			  \n
  *			  change@/kernel/DualWave/sound		\n
  *			  ACTION=change						\n
  *			  DEVPATH=/kernel/DualWave/sound	\n
  *			  SUBSYSTEM=DualWave				\n
  *			  CAPTURE_TIME=2.000000001			\n
  *			  SEQNUM=3260						\n
  *
  * @param	char *szName		the type of time info  (PLAY_TIME or CAPTURE_TIME)
  * @param	timespec *ptTime	the time information
  * @return	static
  * @warning
  */
int send_uevent_wh_timeinfo(const char *szName, struct timespec *ptTime)
{
	char *pEnvp[2];
	char *prBuf;
	int retval;

#if 0	// this flag will be checked the caller side
	if(likely(g_iEnableUpdateSoundTime == DUALWAVE_FALSE))
	{
		return 0;
	}
#endif

	printk(KERN_INFO "khhan %s %d\n", __func__, __LINE__);

	prBuf = kzalloc(MAX_DUALWAVE_MESSAGE_SIZE, GFP_KERNEL);
	if(prBuf == NULL)
	{
		printk(KERN_INFO "khhan ***** failed to alloc memory for dualwave message buffer %s %d ***** \n", __func__, __LINE__);
		return -1;
	}
	pEnvp[0] = prBuf;
	pEnvp[1] = NULL;

	sprintf(prBuf, PRINT_UEVNET_FORMAT_STRING,szName, ptTime->tv_sec, ptTime->tv_nsec);

	retval =  kobject_uevent_env(g_ptDualWaveKobj, KOBJ_CHANGE, pEnvp) ;
	if(retval)
	{
		printk(KERN_INFO "khhan ***** failed to call kobject_event (%d) %s %d ***** \n", retval, __func__, __LINE__);
		kfree(prBuf);
		return -1;
	}

	kfree(prBuf);

	return 0 ;
}
EXPORT_SYMBOL(send_uevent_wh_timeinfo);


int send_uevent_wh_ble_info(char *prEnvInfoLists[3])
{
        int retval;

		printk(KERN_INFO "khhan %s %d\n", __func__, __LINE__);
//        prBuf = kzalloc(MAX_DUALWAVE_MESSAGE_SIZE, GFP_KERNEL);
//        if(prBuf == NULL)
//        {
//                printk("[DualWave] failed to alloc memory for dualwave message buffer\n\n");
//                return -1;
//        }
//        pEnvp[0] = prBuf;
//        pEnvp[1] = NULL;

//        sprintf(prBuf, PRINT_UEVNET_FORMAT_STRING,szName, ptTime->tv_sec, ptTime->tv_nsec);

        retval =  kobject_uevent_env(g_ptDualWaveKobj, KOBJ_CHANGE, prEnvInfoLists) ;
        if(retval)
        {
			printk(KERN_INFO "khhan ***** failed to call kobject_event (%d) %s %d ***** \n", retval, __func__, __LINE__);
            return -1;
        }

//        kfree(prBuf);

        return 0 ;
}
EXPORT_SYMBOL(send_uevent_wh_ble_info);



int send_uevent_snd_avail(int state)
{
		char *pEnvp[2];
		char *prBuf;
        int retval;

		printk(KERN_INFO "khhan %s %d\n", __func__, __LINE__);

		prBuf = kzalloc(MAX_DUALWAVE_MESSAGE_SIZE, GFP_KERNEL);
		if(prBuf == NULL)
		{
			printk(KERN_INFO "khhan ***** failed to alloc memory for dualwave message buffer %s %d ***** \n", __func__, __LINE__);
			return -1;
		}
		pEnvp[0] = prBuf;
		pEnvp[1] = NULL;

		sprintf(prBuf, SND_AVAIL_STRING, state);

        retval =  kobject_uevent_env(g_ptDualWaveKobj, KOBJ_CHANGE, pEnvp) ;
        if(retval)
        {
			printk(KERN_INFO "khhan ***** failed to call kobject_event (%d) %s %d ***** \n", retval, __func__, __LINE__);
			kfree(prBuf);
            return -1;
        }

		kfree(prBuf);
        return 0 ;
}
EXPORT_SYMBOL(send_uevent_snd_avail);


/**
  * @fn		static int __init dualwave_init(void)
  * @brief	register module,
  *
  * @param	void
  * @return	int
  * @warning	the code for registering to udev is not implemented
  */
static int __init dualwave_init(void)
{
//	struct kobj_uevent_env *env;
	int retval;
	static const char *SOUND_DIR="sound";

	printk(KERN_INFO "khhan Entering %s %d\n", __func__, __LINE__);

	init_timer(&expTimer);
	expTimer.expires = jiffies + TIMER_VALUE;
	expTimer.data = (unsigned long) NULL;
	expTimer.function = expTimer_service;
	add_timer(&expTimer);

	// Create kset: this is located in /sys/kernel/DualWave
	// This is required to send uevent. if kset is not existed, kobecjt_uevent will be failed
	g_ptDualWaveKset = kset_create_and_add("DualWave", NULL, kernel_kobj);
	//g_ptDualWaveKset = kset_create_and_add("DualWave", NULL, mkobj);

	if (!g_ptDualWaveKset)
	{
		printk(KERN_INFO "khhan ***** failed to execute kobject_create_and_add %s %d ***** \n", __func__, __LINE__);
		err_status = -1;
		return -EEXIST;
	}

	g_ptDualWaveKobj = kzalloc(sizeof(*g_ptDualWaveKobj), GFP_KERNEL);

	if(g_ptDualWaveKobj == NULL)
	{
		printk(KERN_INFO "khhan ***** failed to kobj kzalloc %s %d ***** \n", __func__, __LINE__);
		err_status = -2;
		return -ENOMEM;
	}

	// set the parent
	g_ptDualWaveKobj->kset = g_ptDualWaveKset;

	// Create kobject: this is located in /sys/kernel/DualWave/sound
	retval = kobject_init_and_add(g_ptDualWaveKobj,&g_tKobjType,NULL,"%s",SOUND_DIR);

	if (retval)
	{
		kset_put(g_ptDualWaveKset);
		printk(KERN_INFO "khhan ***** failed to kobj init and add %s %d ***** \n", __func__, __LINE__);
		err_status = -3;
		kfree(g_ptDualWaveKobj);
		return -1;
	}

	// send uevent for testing
	retval = kobject_uevent(g_ptDualWaveKobj, KOBJ_ADD);
	if(retval)
	{
		printk(KERN_INFO "khhan ***** failed to call kobject_event (%d) %s %d ***** \n", retval, __func__, __LINE__);
		err_status = -4;
		kfree(g_ptDualWaveKobj);
		return -1;
	}


	err_status = 0;
    return 0;
}

/**
  * @fn		static void __exit dualwave_exit(void){
  * @brief	remove module
  *
  * @param	void
  * @return	static
  * @warning
  */
static void __exit dualwave_exit(void){
    printk(KERN_INFO "khhan exiting module %s %d \n", __func__, __LINE__);

	del_timer_sync(&expTimer);
	kobject_put(g_ptDualWaveKobj);
	kset_put(g_ptDualWaveKset);
	kfree(g_ptDualWaveKobj);
}

module_init(dualwave_init);
module_exit(dualwave_exit);

MODULE_LICENSE("GPL");
