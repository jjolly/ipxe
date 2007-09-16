#ifndef _ARBEL_H
#define _ARBEL_H

/** @file
 *
 * Mellanox Arbel Infiniband HCA driver
 *
 */

/*
 * Hardware constants
 *
 */

#define ARBEL_OPCODE_SEND		0x0a
#define ARBEL_OPCODE_RECV_ERROR		0xfe
#define ARBEL_OPCODE_SEND_ERROR		0xff

/*
 * Wrapper structures for hardware datatypes
 *
 */

struct MLX_DECLARE_STRUCT ( arbelprm_completion_queue_context );
struct MLX_DECLARE_STRUCT ( arbelprm_completion_queue_entry );
struct MLX_DECLARE_STRUCT ( arbelprm_completion_with_error );
struct MLX_DECLARE_STRUCT ( arbelprm_cq_ci_db_record );
struct MLX_DECLARE_STRUCT ( arbelprm_hca_command_register );
struct MLX_DECLARE_STRUCT ( arbelprm_qp_db_record );
struct MLX_DECLARE_STRUCT ( arbelprm_query_dev_lim );
struct MLX_DECLARE_STRUCT ( arbelprm_recv_wqe_segment_next );
struct MLX_DECLARE_STRUCT ( arbelprm_send_doorbell );
struct MLX_DECLARE_STRUCT ( arbelprm_ud_address_vector );
struct MLX_DECLARE_STRUCT ( arbelprm_wqe_segment_ctrl_send );
struct MLX_DECLARE_STRUCT ( arbelprm_wqe_segment_data_ptr );
struct MLX_DECLARE_STRUCT ( arbelprm_wqe_segment_next );
struct MLX_DECLARE_STRUCT ( arbelprm_wqe_segment_ud );

/*
 * Composite hardware datatypes
 *
 */

#define ARBEL_MAX_GATHER 1

struct arbelprm_ud_send_wqe {
	struct arbelprm_wqe_segment_next next;
	struct arbelprm_wqe_segment_ctrl_send ctrl;
	struct arbelprm_wqe_segment_ud ud;
	struct arbelprm_wqe_segment_data_ptr data[ARBEL_MAX_GATHER];
} __attribute__ (( packed ));

#define ARBEL_MAX_SCATTER 1

struct arbelprm_recv_wqe {
	/* The autogenerated header is inconsistent between send and
	 * receive WQEs.  The "ctrl" structure for receive WQEs is
	 * defined to include the "next" structure.  Since the "ctrl"
	 * part of the "ctrl" structure contains only "reserved, must
	 * be zero" bits, we ignore its definition and provide
	 * something more usable.
	 */
	struct arbelprm_recv_wqe_segment_next next;
	uint32_t ctrl[2]; /* All "reserved, must be zero" */
	struct arbelprm_wqe_segment_data_ptr data[ARBEL_MAX_SCATTER];
} __attribute__ (( packed ));

union arbelprm_completion_entry {
	struct arbelprm_completion_queue_entry normal;
	struct arbelprm_completion_with_error error;
} __attribute__ (( packed ));

union arbelprm_doorbell_record {
	struct arbelprm_cq_ci_db_record cq_ci;
	struct arbelprm_qp_db_record qp;
} __attribute__ (( packed ));

union arbelprm_doorbell_register {
	struct arbelprm_send_doorbell send;
	uint32_t dword[2];
} __attribute__ (( packed ));

/*
 * gPXE-specific definitions
 *
 */

/** Alignment of Arbel send work queue entries */
#define ARBEL_SEND_WQE_ALIGN 128

/** An Arbel send work queue entry */
union arbel_send_wqe {
	struct arbelprm_ud_send_wqe ud;
	uint8_t force_align[ARBEL_SEND_WQE_ALIGN];
} __attribute__ (( packed ));

/** An Arbel send work queue */
struct arbel_send_work_queue {
	/** Doorbell record number */
	unsigned int doorbell_idx;
	/** Work queue entries */
	union arbel_send_wqe *wqe;
};

/** Alignment of Arbel receive work queue entries */
#define ARBEL_RECV_WQE_ALIGN 64

/** An Arbel receive work queue entry */
union arbel_recv_wqe {
	struct arbelprm_recv_wqe recv;
	uint8_t force_align[ARBEL_RECV_WQE_ALIGN];
} __attribute__ (( packed ));

/** An Arbel receive work queue */
struct arbel_recv_work_queue {
	/** Doorbell record number */
	unsigned int doorbell_idx;
	/** Work queue entries */
	union arbel_recv_wqe *wqe;
};

/** An Arbel completion queue */
struct arbel_completion_queue {
	/** Doorbell record number */
	unsigned int doorbell_idx;
	/** Completion queue entries */
	union arbelprm_completion_entry *cqe;
};

/** An Arbel device */
struct arbel {
	/** Configuration registers */
	void *config;
	/** Command input mailbox */
	void *mailbox_in;
	/** Command output mailbox */
	void *mailbox_out;

	/** User Access Region */
	void *uar;
	/** Doorbell records */
	union arbelprm_doorbell_record *db_rec;
	/** Reserved LKey
	 *
	 * Used to get unrestricted memory access.
	 */
	unsigned long reserved_lkey;
	
};

/*
 * HCA commands
 *
 */

#define ARBEL_HCR_QUERY_DEV_LIM		0x0003

#define ARBEL_HCR_BASE			0x80680
#define ARBEL_HCR_REG(x)		( ARBEL_HCR_BASE + 4 * (x) )
#define ARBEL_HCR_MAX_WAIT_MS		2000

/* HCA command is split into
 *
 * bits  11:0	Opcode
 * bit     12	Input uses mailbox
 * bit     13	Output uses mailbox
 * bits 22:14	Input parameter length (in dwords)
 * bits 31:23	Output parameter length (in dwords)
 *
 * Encoding the information in this way allows us to cut out several
 * parameters to the arbel_command() call.
 */
#define ARBEL_HCR_IN_MBOX		0x00001000UL
#define ARBEL_HCR_OUT_MBOX		0x00002000UL
#define ARBEL_HCR_OPCODE( _command )	( (_command) & 0xfff )
#define ARBEL_HCR_IN_LEN( _command )	( ( (_command) >> 12 ) & 0x7fc )
#define ARBEL_HCR_OUT_LEN( _command )	( ( (_command) >> 21 ) & 0x7fc )

/** Build HCR command from component parts */
#define ARBEL_HCR_CMD( _opcode, _in_mbox, _in_len, _out_mbox, _out_len )     \
	( (_opcode) |							     \
	  ( (_in_mbox) ? ARBEL_HCR_IN_MBOX : 0 ) |			     \
	  ( ( (_in_len) / 4 ) << 14 ) |					     \
	  ( (_out_mbox) ? ARBEL_HCR_OUT_MBOX : 0 ) |			     \
	  ( ( (_out_len) / 4 ) << 23 ) )

#define ARBEL_HCR_IN_CMD( _opcode, _in_mbox, _in_len )			     \
	ARBEL_HCR_CMD ( _opcode, _in_mbox, _in_len, 0, 0 )

#define ARBEL_HCR_OUT_CMD( _opcode, _out_mbox, _out_len )		     \
	ARBEL_HCR_CMD ( _opcode, 0, 0, _out_mbox, _out_len )

#endif /* _ARBEL_H */
