/* The following names are synonyms for the variables in the input message. */
#define m_addr			m1_p1
#define m_exec_name		m1_p1
#define m_exec_len		m1_i1
#define m_func			m6_f1
#define m_group_id		m1_i1
#define m_name_len		m1_i2
#define m_proc_id		m1_i1
#define	m_proc_num		m1_i1
#define m_seconds		m1_i1
#define	m_sig			m6_i1
#define m_stack_bytes	m1_i2
#define m_stack_ptr		m1_p2
#define m_status		m1_i1
#define m_user_id		m1_i1
#define	m_request		m2_i2
#define m_taddr			m2_l1
#define	m_data			m2_l2
#define	m_sig_num		m1_i2
#define	m_sig_new_sa	m1_p1
#define m_sig_old_sa	m1_p2
#define m_sig_return	m1_p3
#define m_sig_set		m2_l1
#define m_sig_how		m2_i1
#define m_sig_flags		m2_i2
#define m_sig_context	m2_p1
#define m_info_what		m1_i1
#define m_info_where	m1_p1
#define m_reboot_flag	m1_i1
#define	m_reboot_code	m1_p1
#define m_reboot_str_len	m1_i2
#define m_svrctl_req	m2_i1
#define m_svrctl_argp	m2_p1
#define m_stime			m2_l1
#define	m_mem_size		m4_l1
#define	m_mem_base		m4_l2

/* The following names are synonyms for the variables in a reply message. */
#define m_reply_res		m_type
#define m_reply_res2	m2_i1
#define m_reply_ptr		m2_p1
#define m_reply_mask	m2_l1
#define m_reply_trace	m2_l2
#define m_reply_time	m2_l1
#define m_reply_utime	m2_l2
#define m_reply_t1		m4_l1
#define m_reply_t2		m4_l2
#define m_reply_t3		m4_l3
#define m_reply_t4		m4_l4
#define m_reply_t5		m4_l5

/* The following names are used to inform the FS about certain events. */
#define m_tell_fs_arg1	m1_i1
#define m_tell_fs_arg2	m1_i2
#define m_tell_fs_arg3	m1_i3
