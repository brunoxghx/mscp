/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef _MSCP_H_
#define _MSCP_H_

/**
 * @file mscp.h
 *
 * @brief mscp library header file.
 *
 * @mainpage
 *
 * libmscp is a library for multi-threaded scp. Project page is
 * https://github.com/upa/mscp.
 *
 * All public APIs of libmscp are defined in mscp.h. Basic usage of
 * libmscp is follows:
 *
 * 1. create mscp instance with mscp_init()
 * 2. connect to remote host with mscp_connect()
 * 3. add path to source files with mscp_add_src_path()
 * 4. set path to destination with mscp_set_dst_path()
 * 5. start to scan source files with mscp_scan()
 * 6. start copy with mscp_start()
 * 7. wait for copy finished with mscp_join()
 * 8. cleanup mscp instance with mscp_cleanup() and mscp_free()
 */

#include <stdbool.h>
#include <limits.h>

#define MSCP_DIRECTION_L2R	1	/** Indicates local to remote copy */
#define MSCP_DIRECTION_R2L	2	/** Indicates remote to local copy */

/**
 * @struct	mscp_opts
 * @brief	Structure configuring mscp.
 */
struct mscp_opts {
	int	nr_threads;	/** number of copy threads */
	int	nr_ahead;	/** number of SFTP commands on-the-fly */
	size_t	min_chunk_sz;	/** minimum chunk size (default 64MB) */
	size_t	max_chunk_sz;	/** maximum chunk size (default file size/nr_threads) */
	size_t	buf_sz;		/** buffer size, default 16k. */
	char	*coremask; /** hex to specifiy usable cpu cores */
	int	max_startups;	/** sshd MaxStartups concurrent connections */
	int     interval;	/** interval between SSH connection attempts */
	bool	preserve_ts;	/** preserve file timestamps */

	int	severity; 	/** messaging severity. set MSCP_SERVERITY_* */
};


/**
 * @struct	mscp_ssh_opts
 * @brief	Structure configuring SSH connections
 */
struct mscp_ssh_opts {
	/* ssh options */
	char	*login_name;	/** ssh username */
	char	*port;		/** ssh port */
	int	ai_family;	/** address family */
	char	*config;	/** path to ssh_config, default ~/.ssh/config*/
	char	*identity;	/** path to private key */
	char	*cipher;	/** cipher spec */
	char	*hmac;		/** hmacp spec */
	char	*compress;	/** yes, no, zlib@openssh.com */
	char	*ccalgo;	/** TCP cc algorithm */

	char	*password;	/** password auth passowrd */
	char	*passphrase;	/** passphrase for private key */

	int	debug_level;		/** inclirement libssh debug output level */
	bool	no_hostkey_check;	/** do not check host keys */
	bool	enable_nagle;		/** enable Nagle's algorithm if true */
};

/** @def
 * Environment variable that passes password for ssh password auth
 */
#define ENV_SSH_AUTH_PASSWORD	"MSCP_SSH_AUTH_PASSWORD"

/** @def
 * Environment vraible that passes passphrase for private key
 */
#define ENV_SSH_AUTH_PASSPHRASE	"MSCP_SSH_AUTH_PASSPHRASE"


/**
 * @struct	mscp_stats
 * @brief	Structure to get mscp statistics
 */
struct mscp_stats {
	size_t total;	/** total bytes to be transferred */
	size_t done;	/** total bytes transferred */
};


/** Structure representing mscp instance */
struct mscp;

/**
 * @brief Creates a new mscp instance.
 *
 * @param remote_host	remote host for file transer.
 * @param direction	copy direction, `MSCP_DIRECTION_L2R` or `MSCP_DIRECTION_R2L`
 * @param o		options for configuring mscp.
 * @param s		options for configuring ssh connections.
 *
 * @retrun 		A new mscp instance or NULL on error.
 */
struct mscp *mscp_init(const char *remote_host, int direction,
		       struct mscp_opts *o, struct mscp_ssh_opts *s);

/**
 * @brief Connect the first SSH connection. mscp_connect connects to
 * remote host and initialize a SFTP session over the
 * connection. mscp_scan() and mscp_start() require mscp_connect()
 * beforehand.
 *
 * @param m	mscp instance.
 *
 * @return 	0 on success, < 0 if an error occured.
 */
int mscp_connect(struct mscp *m);

/* add a source file path to be copied */

/**
 * @brief Add a source file path to be copied. The path indicates
 * either a file or directory.
 *
 * @param m		mscp instance.
 * @param src_path	source file path to be copied.
 *
 * @return 		0 on success, < 0 if an error occured.
 */
int mscp_add_src_path(struct mscp *m, const char *src_path);

/**
 * @brief Set the destination file path. The path indicates either a
 * file, directory, or nonexistent path.
 *
 * @param m		mscp instance.
 * @param dst_path	destination path to which source files copied.
 *
 * @return 		0 on success, < 0 if an error occured.
 */
int mscp_set_dst_path(struct mscp *m, const char *dst_path);

/* scan source files, resolve destination file paths for all source
 * files, and calculate chunks for all files. */

/**
 * @brief Scan source paths and prepare. This function checks all
 * source files (recursively), resolve paths on the destination side,
 * and calculate file chunks. This function is non-blocking.
 *
 * @param m	mscp instance.
 *
 * @return 	0 on success, < 0 if an error occured.
 */
int mscp_scan(struct mscp *m);

/**
 * @brief Join scna thread invoked by mscp_scan(). mscp_join()
 * involves this, so that mscp_scan_join() should be called when
 * mscp_scan() is called by mscp_start() is not.
 *
 * @param m	mscp instance.
 * @return	0 on success, < 0 if an error occured.
 */
int mscp_scan_join(struct mscp *m);

/**
 * @brief Start to copy files. mscp_start() returns immediately. You
 * can get statistics via mscp_get_stats() or messages via pipe set by
 * mscp_opts.msg_fd or mscp_set_msg_fd(). mscp_stop() cancels mscp
 * copy threads, and mscp_join() joins the threads.
 *
 * @param m	mscp instance.
 *
 * @return 	number of threads on success, < 0 if an error occured.
 *
 * @see		mscp_join()
 */
int mscp_start(struct mscp *m);


/**
 * @brief Stop coping files.
 *
 * @param m	mscp instance.
 */
void mscp_stop(struct mscp *m);


/**
 * @brief Join copy threads. This function is blocking until all copy
 * have done.
 *
 * @param m	mscp instance.
 *
 * @return 	0 on success, < 0 if an error occured.
 */
int mscp_join(struct mscp *m);

/**
 * @brief Get statistics of copy.
 *
 * @param m		mscp instance.
 * @param s[out]	statistics.
 */
void mscp_get_stats(struct mscp *m, struct mscp_stats *s);

/**
 * @brief Cleanup the mscp instance. Before calling mscp_cleanup(),
 * must call mscp_join(). After mscp_cleanup() called, the mscp
 * instance can restart from mscp_connect(). Note that do not call
 * mscp_cleanup() before callign mscp_join(). It causes crash (ToDo:
 * check status of copy threads and return error when they are
 * running).
 *
 * @param m		mscp instance.
 */
void mscp_cleanup(struct mscp *m);

/**
 * @brief Release the mscp instance.  Note that do not call *
 mscp_free() before calling mscp_join(). It causes crash (ToDo: check
 * status of copy threads and return error when they are running).
 *
 * @param m		mscp instance.
 */
void mscp_free(struct mscp *m);


/* messaging with mscp */

/**
 * @enum	mscp_serverity
 * @brief 	Filter messages from libmscp with severity level.
 */
enum {
	MSCP_SEVERITY_NONE	= -1,
	MSCP_SEVERITY_ERR	= 0,
	MSCP_SEVERITY_WARN	= 1,
	MSCP_SEVERITY_NOTICE	= 2,
        MSCP_SEVERITY_INFO	= 3,
	MSCP_SEVERITY_DEBUG	= 4,
};


#endif /* _MSCP_H_ */
