/* The following names are synonyms for the variables in the input message. */

#define m_addr			m1_i3
#define m_buffer		m1_p1
#define m_child			m1_i2
#define m_mask			m1_i1
#define m_egid			m1_i3
#define m_euid			m1_i3

#define m_fd			m1_i1
#define m_fd2			m1_i2

#define m_group			m1_i3
#define m_rgid			m1_i2
#define m_ls_fd			m2_i1


#define m_mode			m3_i2
#define m_c_mode		m1_i3
#define m_c_name		m1_p1
#define m_name			m3_p1
#define	m_name1			m1_p1
#define m_name2			m1_p2
#define m_name_length	m3_i1
#define m_name1_length	m1_i1
#define m_name2_length	m1_i2
#define m_nbytes		m1_i2
#define m_owner			m1_i2
#define m_parent		m1_i1
#define m_path_name		m3_ca1
#define m_pid			m1_i3
#define m_pro			m1_i1
#define m_ctl_req		m4_l1
#define m_driver_num	m4_l2
#define m_dev_num		m4_l3
#define m_dev_style		m4_l4
#define m_rd_only		m1_i3
#define m_ruid			m1_i2
#define m_request		m1_i2

#define m_slot1			m1_i1

#define m_utime_actime	m2_l1
#define m_utime_modtime	m2_l2
#define m_utime_file	m2_p1
#define m_utime_length	m2_i1
#define m_utime_strlen	m2_i2
#define m_whence		m2_i2


#define m_pm_stime		m1_i1


#define m_offset		m2_l1
/* The following names are synonums for the variables in the output message. */
#define m_reply_type	m_type
#define m_reply_l1		m2_l1
#define m_reply_i1		m1_i1
#define m_reply_i2		m1_i2
