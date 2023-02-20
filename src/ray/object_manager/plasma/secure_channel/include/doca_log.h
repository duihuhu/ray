/*
 * Copyright (c) 2021 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
 *
 * This software product is a proprietary product of NVIDIA CORPORATION &
 * AFFILIATES (the "Company") and all right, title, and interest in and to the
 * software product, including all associated intellectual property rights, are
 * and shall remain exclusively with the Company.
 *
 * This software product is governed by the End User License Agreement
 * provided with the software product.
 *
 */

/**
 * @file doca_log.h
 * @page logger
 * @defgroup LOGGER Logging Management
 *
 * Define functions for internal and external logging management
 *
 * To add DOCA internal logging compile with "-D DOCA_LOGGING_ALLOW_DLOG"
 *
 * @{
 */

#ifndef DOCA_LOG_H_
#define DOCA_LOG_H_

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

#include <doca_compat.h>
#include <doca_error.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief log levels
 */
enum doca_log_level {
	DOCA_LOG_LEVEL_CRIT = 20,	/**< Critical log level */
	DOCA_LOG_LEVEL_ERROR = 30,	/**< Error log level */
	DOCA_LOG_LEVEL_WARNING = 40,	/**< Warning log level */
	DOCA_LOG_LEVEL_INFO = 50,	/**< Info log level */
	DOCA_LOG_LEVEL_DEBUG = 60	/**< Debug log level */
};

/**
 * @brief logging backend
 */
struct doca_logger_backend;

/**
 * @brief logging backend flush() handler
 */
typedef void (*log_flush_callback)(char *buffer);

/**
 * @brief Redirect the logger to a different stream.
 *
 * Dynamically change the logger stream of the default logger backend. The default
 * stream is stderr.
 *
 * @param[in] stream
 * Pointer to the stream.
 * @return
 * DOCA error code.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_log_stream_redirect(FILE *stream);

/**
 * @brief Get the timespan of the rate-limit bucket.
 *
 * @return
 * Time (in seconds) of the rate-limit bucket.
 */
__DOCA_EXPERIMENTAL
uint16_t doca_log_get_bucket_time(void);

/**
 * @brief Set the timespan of the rate-limit bucket.
 *
 * @param[in] bucket_time
 * Time (in seconds) for the rate-limit bucket.
 */
__DOCA_EXPERIMENTAL
void doca_log_set_bucket_time(uint16_t bucket_time);

/**
 * @brief Get the quantity of the rate-limit bucket.
 *
 * @return
 * Maximal number of log events for a rate-limit bucket.
 */
__DOCA_EXPERIMENTAL
uint16_t doca_log_get_quantity(void);

/**
 * @brief Set the quantity of the rate-limit bucket.
 *
 * @param quantity
 * Maximal number of log events for a rate-limit bucket.
 */
__DOCA_EXPERIMENTAL
void doca_log_set_quantity(uint16_t quantity);

/**
 * @brief Set the log level of a specific logger backend.
 *
 * Dynamically change the log level of the given logger backend, any log under this
 * level will be shown.
 *
 * @param[in] logger
 * Logger backend to update.
 * @param[in] level
 * Log level enum DOCA_LOG_LEVEL.
 * @return
 * DOCA error code.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_log_backend_level_set(struct doca_logger_backend *logger, uint32_t level);

/**
 * @brief Set the log level of the default logger backend.
 *
 * Dynamically change the log level of the default logger backend, any log under this
 * level will be shown.
 *
 * @param[in] level
 * Log level enum DOCA_LOG_LEVEL.
 * @return
 * DOCA error code.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_log_global_level_set(uint32_t level);

/**
 * @brief Get the log level of the default logger backend.
 *
 * Dynamically query for the log level of the default logger backend, any log under this
 * level will be shown.
 *
 * @return
 * Log level enum DOCA_LOG_LEVEL.
 */
__DOCA_EXPERIMENTAL
uint32_t doca_log_global_level_get(void);

/**
 * @brief Register a log source.
 *
 * Will return the identifier associated with the log source. Log source name will be shown
 * in the logs.
 *
 * @note Recommended to only be used via DOCA_LOG_REGISTER.
 *
 * @param[in] source_name
 * The string identifying the log source. Should be in an heirarchic form (i.e. DPI::Parser).
 * @param[out] source
 * Source identifier that was allocated to this log source name (only valid if no error occurred).
 * @return
 * DOCA error code.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_log_source_register(const char *source_name, int *source);

/**
 * @brief Destroy a log source.
 *
 * Destroys a given log source as part of the teardown process of the running program.
 *
 * @note Used automatically via DOCA_LOG_REGISTER, not recommended to call it directly.
 *
 * @param[in] source
 * The source identifier of source to be destroyed, as allocated by doca_log_source_register.
 * @return
 * DOCA error code.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_log_source_destroy(int source);

/**
 * @brief Register a new rate bucket.
 *
 * Will return the identifier associated with the new bucket.
 *
 * @param[in] source
 * The log source identifier defined by doca_log_source_register.
 * @param[out] bucket
 * Bucket identifier that was allocated to this log source (only valid if no error occurred).
 * @return
 * DOCA error code.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_log_rate_bucket_register(int source, int *bucket);

/**
 * @brief Create a logging backend with a FILE* stream.
 *
 * Creates a new logging backend that will be added on top of the default logger.
 *
 * @param[in] fptr
 * The FILE * for the logger's stream.
 * @param[out] backend
 * Logging backend that wraps the given fptr (only valid if no error occurred).
 * @return
 * DOCA error code.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_log_create_file_backend(FILE *fptr, struct doca_logger_backend **backend);

/**
 * @brief Create a logging backend with an fd stream.
 *
 * Creates a new logging backend that will be added on top of the default logger.
 *
 * @param[in] fd
 * The file descriptor (int) for the logger's backend.
 * @param[out] backend
 * Logging backend that wraps the given fd (only valid if no error occurred).
 * @return
 * DOCA error code.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_log_create_fd_backend(int fd, struct doca_logger_backend **backend);

/**
 * @brief Create a logging backend with a char buffer stream.
 *
 * Creates a new logging backend that will be added on top of the default logger. The
 * logger will write each log record at the beginning of this buffer.
 *
 * @param[in] buffer
 * The char buffer (char *) for the logger's stream.
 * @param[in] capacity
 * Maximal amount of chars that could be written to the stream.
 * @param[in] handler
 * Handler to be called when the log record should be flushed from the stream.
 * @param[out] backend
 * Logging backend that wraps the given buffer (only valid if no error occurred).
 * @return
 * DOCA error code.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_log_create_buffer_backend(char *buffer, size_t capacity, log_flush_callback handler,
					    struct doca_logger_backend **backend);

/**
 * @brief Create a logging backend with a syslog output.
 *
 * Creates a new logging backend that will be added on top of the default logger.
 *
 * @param[in] name
 * The syslog name for the logger's backend.
 * @param[out] backend
 * Logging backend that exposes the desired syslog functionality (only valid if no error occurred).
 * @return
 * DOCA error code.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_log_create_syslog_backend(const char *name, struct doca_logger_backend **backend);

/**
 * @brief Generates a log message.
 *
 * The log will be shown in the doca_log_stream_redirect (see default).
 * This should not be used, please prefer using DOCA_LOG...
 *
 * @param[in] level
 * Log level enum DOCA_LOG_LEVEL.
 * @param[in] source
 * The log source identifier defined by doca_log_source_register.
 * @param[in] line
 * The line number this log originated from.
 * @param[in] format
 * printf(3) arguments, format and variables.
 * @return
 * DOCA error code.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_log(uint32_t level, int source, int line, const char *format, ...) __attribute__ ((format (printf, 4, 5)));

/**
 * @brief Generates a log message for DLOG operations.
 *
 * The log will be shown in the doca_log_stream_redirect (see default).
 * @note This function is thread safe.
 *
 * @param[in] level
 * Log level enum DOCA_LOG_LEVEL.
 * @param[in] source
 * The log source identifier defined by doca_log_source_register.
 * @param[in] line
 * The line number this log originated from.
 * @param[in] format
 * printf(3) arguments, format and variables.
 * @return
 * DOCA error code.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_log_developer(uint32_t level, int source, int line, const char *format, ...) __attribute__ ((format (printf, 4, 5)));

/**
 * @brief Generates a log message with rate limit.
 *
 * The log will be shown in the doca_log_stream_redirect (see default).
 * This should not be used, please prefer using DOCA_LOG_RATE_LIMIT...
 *
 * @param level
 * Log level enum DOCA_LOG_LEVEL.
 * @param source
 * The log source identifier defined by doca_log_source_register.
 * @param line
 * The line number this log originated from.
 * @param bucket
 * The bucket identifier defined by doca_log_rate_bucket_register.
 * @param format
 * printf(3) arguments, format and variables.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_log_rate_limit(uint32_t level, int source, int line, int bucket, const char *format, ...)
    __attribute__((format(printf, 5, 6)));

/**
 * @brief Generates a log message with rate limit.
 *
 * The DOCA_LOG_RATE_LIMIT calls DOCA_LOG with some rate limit. Implied
 * to be used on hot paths.
 *
 * @param level
 * Log level enum DOCA_LOG_LEVEL (just ERROR, WARNING...).
 * @param format
 * printf(3) arguments, format and variables.
 */
#define DOCA_LOG_RATE_LIMIT(level, format, ...)                                                                        \
	do {                                                                                                           \
		static int log_bucket = -1;                                                                            \
		if (log_bucket == -1) {                                                                                \
			doca_log_rate_bucket_register(log_source, &log_bucket);                                        \
		}                                                                                                      \
		doca_log_rate_limit(DOCA_LOG_LEVEL_##level, log_source, __LINE__, log_bucket, format, ##__VA_ARGS__);  \
	} while (0)

/**
 * @brief Generates a CRITICAL rate limited log message.
 *
 * @param format
 * printf(3) arguments, format and variables.
 */
#define DOCA_LOG_RATE_LIMIT_CRIT(format, ...) DOCA_LOG_RATE_LIMIT(CRIT, format, ##__VA_ARGS__)

/**
 * @brief Generates an ERROR rate limited log message.
 *
 * @param format
 * printf(3) arguments, format and variables.
 */
#define DOCA_LOG_RATE_LIMIT_ERR(format, ...) DOCA_LOG_RATE_LIMIT(ERROR, format, ##__VA_ARGS__)

/**
 * @brief Generates a WARNING rate limited log message.
 *
 * @param format
 * printf(3) arguments, format and variables.
 */
#define DOCA_LOG_RATE_LIMIT_WARN(format, ...) DOCA_LOG_RATE_LIMIT(WARNING, format, ##__VA_ARGS__)

/**
 * @brief Generates an INFO rate limited log message.
 *
 * @param format
 * printf(3) arguments, format and variables.
 */
#define DOCA_LOG_RATE_LIMIT_INFO(format, ...) DOCA_LOG_RATE_LIMIT(INFO, format, ##__VA_ARGS__)

/**
 * @brief Generates a DEBUG rate limited log message.
 *
 * @param format
 * printf(3) arguments, format and variables.
 */

#define DOCA_LOG_RATE_LIMIT_DBG(format, ...) DOCA_LOG_RATE_LIMIT(DEBUG, format, ##__VA_ARGS__)
/**
 * @brief Generates a log message.
 *
 * The DOCA_LOG() is the main log function for logging. This call affects the performance.
 * Consider using DOCA_DLOG for the option to remove it on the final compilation.
 * Consider using the specific level DOCA_LOG for better code readability (i.e. DOCA_LOG_ERR).
 *
 * @param level
 * Log level enum DOCA_LOG_LEVEL (just ERROR, WARNING...).
 * @param format
 * printf(3) arguments, format and variables.
 */
#define DOCA_LOG(level, format, ...) doca_log(DOCA_LOG_LEVEL_##level, log_source, __LINE__, format, ##__VA_ARGS__)

/**
 * @brief Generates a CRITICAL log message.
 *
 * Will generate critical log. This call affects the performance.
 * Consider using DOCA_DLOG for the option to remove it on the final compilation.
 *
 * @param format
 * printf(3) arguments, format and variables.
 */
#define DOCA_LOG_CRIT(format, ...) DOCA_LOG(CRIT, format, ##__VA_ARGS__)

/**
 * @brief Generates an ERROR log message.
 *
 * Will generate error log. This call affects the performance.
 * Consider using DOCA_DLOG for the option to remove it on the final compilation.
 *
 * @param format
 * printf(3) arguments, format and variables.
 */
#define DOCA_LOG_ERR(format, ...) DOCA_LOG(ERROR, format, ##__VA_ARGS__)

/**
 * @brief Generates a WARNING log message.
 *
 * Will generate warning log. This call affects the performace.
 * Consider using DOCA_DLOG for the option to remove it on the final compilation.
 *
 * @param format
 * printf(3) arguments, format and variables.
 */
#define DOCA_LOG_WARN(format, ...) DOCA_LOG(WARNING, format, ##__VA_ARGS__)

/**
 * @brief Generates an INFO log message.
 *
 * Will generate info log. This call affects the performance.
 * Consider using DOCA_DLOG for the option to remove it on the final compilation.
 *
 * @param format
 * printf(3) arguments, format and variables.
 */
#define DOCA_LOG_INFO(format, ...) DOCA_LOG(INFO, format, ##__VA_ARGS__)

/**
 * @brief Generates a DEBUG log message.
 *
 * Will generate debug log. This call affects the performace.
 * Consider using DOCA_DLOG for the option to remove it on the final compilation.
 *
 * @param format
 * printf(3) arguments, format and variables.
 */
#define DOCA_LOG_DBG(format, ...) DOCA_LOG(DEBUG, format, ##__VA_ARGS__)

#ifdef DOCA_LOGGING_ALLOW_DLOG

/**
 * @brief Generates a development log message.
 *
 * The DOCA_DLOG() is the main log function for development purposes logging.
 * To show the logs, define DOCA_LOGGING_ALLOW_DLOG in the compilation variables.
 * This will not effect performance if compiled without DOCA_LOGGING_ALLOW_DLOG, as
 * it will be removed by the compiler.
 * Consider using the specific level DOCA_LOG for better code readability (i.e. DOCA_DLOG_ERR).
 *
 * @param level
 * Log level enum DOCA_LOG_LEVEL.
 * @param format
 * printf(3) arguments, format and variables.
 */
#define DOCA_DLOG(level, format, ...) \
    doca_log_developer(DOCA_LOG_LEVEL_##level, log_source, __LINE__, format, ##__VA_ARGS__)

#else

/**
 * @brief Generates a development log message.
 *
 * The DOCA_DLOG() is the main log function for development purposes logging.
 * To show the logs, define DOCA_LOGGING_ALLOW_DLOG in the compilation variables.
 * This will not effect performance if compiled without DOCA_LOGGING_ALLOW_DLOG, as
 * it will be removed by the compiler.
 * Consider using the specific level DOCA_LOG for better code readability (i.e. DOCA_DLOG_ERR).
 *
 * @param level
 * Log level enum DOCA_LOG_LEVEL.
 * @param format
 * printf(3) arguments, format and variables.
 */
#define DOCA_DLOG(level, format, ...)  \
	do { \
	} while (0)

#endif

/**
 * @brief Generates a CRITICAL development log message.
 *
 * Will generate critical log for development purposes.
 * To show the logs define DOCA_LOGGING_ALLOW_DLOG in the compilation variables.
 * This will not effect performance if compiled without DOCA_LOGGING_ALLOW_DLOG, as
 * it will be removed by the compiler.
 *
 * @param format
 * printf(3) arguments, format and variables.
 */
#define DOCA_DLOG_CRIT(format, ...) DOCA_DLOG(CRIT, format, ##__VA_ARGS__)

/**
 * @brief Generates an ERROR development log message.
 *
 * Will generate error log for development purposes.
 * To show the logs define DOCA_LOGGING_ALLOW_DLOG in the compilation variables.
 * This will not effect performance if compiled without DOCA_LOGGING_ALLOW_DLOG, as
 * it will be removed by the compiler.
 *
 * @param format
 * printf(3) arguments, format and variables.
 */
#define DOCA_DLOG_ERR(format, ...) DOCA_DLOG(ERROR, format, ##__VA_ARGS__)

/**
 * @brief Generates a WARNING development log message.
 *
 * Will generate warning log for development purposes.
 * To show the logs define DOCA_LOGGING_ALLOW_DLOG in the compilation variables.
 * This will not effect performance if compiled without DOCA_LOGGING_ALLOW_DLOG, as
 * it will be removed by the compiler.
 *
 * @param format
 * printf(3) arguments, format and variables.
 */
#define DOCA_DLOG_WARN(format, ...) DOCA_DLOG(WARNING, format, ##__VA_ARGS__)

/**
 * @brief Generates an INFO development log message.
 *
 * Will generate info log for development purposes.
 * To show the logs define DOCA_LOGGING_ALLOW_DLOG in the compilation variables.
 * This will not effect performance if compiled without DOCA_LOGGING_ALLOW_DLOG, as
 * it will be removed by the compiler.
 *
 * @param format
 * printf(3) arguments, format and variables.
 */
#define DOCA_DLOG_INFO(format, ...) DOCA_DLOG(INFO, format, ##__VA_ARGS__)

/**
 * @brief Generates a DEBUG development log message.
 *
 * Will generate debug log for development purposes.
 * To show the logs define DOCA_LOGGING_ALLOW_DLOG in the compilation variables.
 * This will not effect performance if compiled without DOCA_LOGGING_ALLOW_DLOG, as
 * it will be removed by the compiler.
 *
 * @param format
 * printf(3) arguments, format and variables.
 */
#define DOCA_DLOG_DBG(format, ...) DOCA_DLOG(DEBUG, format, ##__VA_ARGS__)

/**
 * @brief Registers log source on program start.
 *
 * Should be used to register the log source.
 * For example:
 *
 * DOCA_LOG_REGISTER(dpi)
 *
 * void foo {
 *       DOCA_LOG_INFO("Message");
 * }
 *
 * @note The macro also takes care of the dtor() logic on teardown.
 *
 * @param SOURCE
 * A string representing the source name.
 */

#ifdef __linux__

#define DOCA_LOG_REGISTER(SOURCE)                                                                                      \
	static int log_source;                                                                                         \
	/* Use the highest priority so other Ctors will be able to use the log */                                      \
	static void __attribute__((constructor(101), used)) DOCA_LOG_CTOR_##__FILE__(void)                             \
	{                                                                                                              \
		doca_log_source_register(#SOURCE, &log_source);                                                        \
	}                                                                                                              \
	/* Keep it symmetric */                                                                                        \
	static void __attribute__((destructor(101), used)) DOCA_LOG_DTOR_##__FILE__(void)                              \
	{                                                                                                              \
		doca_log_source_destroy(log_source);                                                                   \
	}

#else /* implicit windows */

#ifdef __cplusplus

class doca_log_registrator
{
public:
	doca_log_registrator(const char* source_name, int &log_source) noexcept
	{
		doca_log_source_register(source_name, &log_source);
		m_log_source = log_source;
	}
	~doca_log_registrator()
	{
		doca_log_source_destroy(m_log_source);
	}

private:
	int m_log_source{0};
};

/*
 * Windows log supports only C++ at the moment.
 */
#define DOCA_LOG_REGISTER(SOURCE)	\
	static int log_source{0};	\
	static doca_log_registrator g_register_struct(#SOURCE, log_source);

#endif /* __cplusplus */

#endif /* __linux__ */

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */

#endif /* DOCA_LOG_H_ */
