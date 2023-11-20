/*
 *   stunnel       TLS offloading and load-balancing proxy
 *   Copyright (C) 1998-2022 Michal Trojnara <Michal.Trojnara@stunnel.org>
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the
 *   Free Software Foundation; either version 2 of the License, or (at your
 *   option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *   See the GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, see <http://www.gnu.org/licenses>.
 *
 *   Linking stunnel statically or dynamically with other modules is making
 *   a combined work based on stunnel. Thus, the terms and conditions of
 *   the GNU General Public License cover the whole combination.
 *
 *   In addition, as a special exception, the copyright holder of stunnel
 *   gives you permission to combine stunnel with free software programs or
 *   libraries that are released under the GNU LGPL and with code included
 *   in the standard release of OpenSSL under the OpenSSL License (or
 *   modified versions of such code, with unchanged license). You may copy
 *   and distribute such a system following the terms of the GNU GPL for
 *   stunnel and the licenses of the other code concerned.
 *
 *   Note that people who make modified versions of stunnel are not obligated
 *   to grant this special exception for their modified versions; it is their
 *   choice whether to do so. The GNU General Public License gives permission
 *   to release a modified version without this exception; this exception
 *   also makes it possible to release a modified version which carries
 *   forward this exception.
 */

#include "prototypes.h"

#if OPENSSL_VERSION_NUMBER >= 0x10101000L
#define DEFAULT_CURVES "X25519:P-256:X448:P-521:P-384"
#else /* OpenSSL version < 1.1.1 */
#define DEFAULT_CURVES "prime256v1"
#endif /* OpenSSL version >= 1.1.1 */

#if defined(_WIN32_WCE) && !defined(CONFDIR)
#define CONFDIR "\\stunnel"
#endif

#define CONFLINELEN (16*1024)

#define INVALID_SSL_OPTION ((long unsigned)-1)

typedef enum {
    CMD_SET_DEFAULTS,   /* set default values */
    CMD_SET_COPY,       /* duplicate from new_service_options */
    CMD_FREE,           /* deallocate memory */
    CMD_SET_VALUE,      /* set a user-specified value */
    CMD_INITIALIZE,     /* initialize the global options or a section */
    CMD_PRINT_DEFAULTS, /* print default values */
    CMD_PRINT_HELP      /* print help */
} CMD;

NOEXPORT int options_file(char *, CONF_TYPE, SERVICE_OPTIONS **);
NOEXPORT int init_section(int, SERVICE_OPTIONS **);
#ifdef USE_WIN32
struct dirent {
    char d_name[MAX_PATH];
};
int scandir(const char *, struct dirent ***,
    int (*)(const struct dirent *),
    int (*)(const struct dirent **, const struct dirent **));
int alphasort(const struct dirent **, const struct dirent **);
#endif
NOEXPORT const char *parse_global_option(CMD, GLOBAL_OPTIONS *, char *, char *);
NOEXPORT const char *parse_service_option(CMD, SERVICE_OPTIONS **, char *, char *);

#ifndef OPENSSL_NO_TLSEXT
NOEXPORT const char *sni_init(SERVICE_OPTIONS *);
NOEXPORT void sni_free(SERVICE_OPTIONS *);
#endif /* !defined(OPENSSL_NO_TLSEXT) */

#if OPENSSL_VERSION_NUMBER>=0x10100000L
NOEXPORT int str_to_proto_version(const char *);
#else /* OPENSSL_VERSION_NUMBER<0x10100000L */
NOEXPORT char *tls_methods_set(SERVICE_OPTIONS *, const char *);
NOEXPORT char *tls_methods_check(SERVICE_OPTIONS *);
#endif /* OPENSSL_VERSION_NUMBER<0x10100000L */

NOEXPORT const char *parse_debug_level(char *, SERVICE_OPTIONS *);

#ifndef OPENSSL_NO_PSK
NOEXPORT PSK_KEYS *psk_read(char *);
NOEXPORT PSK_KEYS *psk_dup(PSK_KEYS *);
NOEXPORT void psk_free(PSK_KEYS *);
#endif /* !defined(OPENSSL_NO_PSK) */

#if OPENSSL_VERSION_NUMBER>=0x10000000L
NOEXPORT TICKET_KEY *key_read(char *, const char *);
NOEXPORT TICKET_KEY *key_dup(TICKET_KEY *);
NOEXPORT void key_free(TICKET_KEY *);
#endif /* OpenSSL 1.0.0 or later */

typedef struct {
    const char *name;
    uint64_t value;
} SSL_OPTION;

static const SSL_OPTION ssl_opts[] = {
#ifdef SSL_OP_ALL
    {"ALL", SSL_OP_ALL},
#endif
#ifdef SSL_OP_ALLOW_CLIENT_RENEGOTIATION
    {"ALLOW_CLIENT_RENEGOTIATION", SSL_OP_ALLOW_CLIENT_RENEGOTIATION},
#endif
#ifdef SSL_OP_ALLOW_NO_DHE_KEX
    {"ALLOW_NO_DHE_KEX", SSL_OP_ALLOW_NO_DHE_KEX},
#endif
#ifdef SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION
    {"ALLOW_UNSAFE_LEGACY_RENEGOTIATION", SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION},
#endif
#ifdef SSL_OP_CIPHER_SERVER_PREFERENCE
    {"CIPHER_SERVER_PREFERENCE", SSL_OP_CIPHER_SERVER_PREFERENCE},
#endif
#ifdef SSL_OP_CISCO_ANYCONNECT
    {"CISCO_ANYCONNECT", SSL_OP_CISCO_ANYCONNECT},
#endif
#ifdef SSL_OP_CLEANSE_PLAINTEXT
    {"CLEANSE_PLAINTEXT", SSL_OP_CLEANSE_PLAINTEXT},
#endif
#ifdef SSL_OP_COOKIE_EXCHANGE
    {"COOKIE_EXCHANGE", SSL_OP_COOKIE_EXCHANGE},
#endif
#ifdef SSL_OP_CRYPTOPRO_TLSEXT_BUG
    {"CRYPTOPRO_TLSEXT_BUG", SSL_OP_CRYPTOPRO_TLSEXT_BUG},
#endif
#ifdef SSL_OP_DISABLE_TLSEXT_CA_NAMES
    {"DISABLE_TLSEXT_CA_NAMES", SSL_OP_DISABLE_TLSEXT_CA_NAMES},
#endif
#ifdef SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS
    {"DONT_INSERT_EMPTY_FRAGMENTS", SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS},
#endif
#ifdef SSL_OP_ENABLE_KTLS
    {"ENABLE_KTLS", SSL_OP_ENABLE_KTLS},
#endif
#ifdef SSL_OP_ENABLE_MIDDLEBOX_COMPAT
    {"ENABLE_MIDDLEBOX_COMPAT", SSL_OP_ENABLE_MIDDLEBOX_COMPAT},
#endif
#ifdef SSL_OP_EPHEMERAL_RSA
    {"EPHEMERAL_RSA", SSL_OP_EPHEMERAL_RSA},
#endif
#ifdef SSL_OP_IGNORE_UNEXPECTED_EOF
    {"IGNORE_UNEXPECTED_EOF", SSL_OP_IGNORE_UNEXPECTED_EOF},
#endif
#ifdef SSL_OP_LEGACY_SERVER_CONNECT
    {"LEGACY_SERVER_CONNECT", SSL_OP_LEGACY_SERVER_CONNECT},
#endif
#ifdef SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER
    {"MICROSOFT_BIG_SSLV3_BUFFER", SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER},
#endif
#ifdef SSL_OP_MICROSOFT_SESS_ID_BUG
    {"MICROSOFT_SESS_ID_BUG", SSL_OP_MICROSOFT_SESS_ID_BUG},
#endif
#ifdef SSL_OP_MSIE_SSLV2_RSA_PADDING
    {"MSIE_SSLV2_RSA_PADDING", SSL_OP_MSIE_SSLV2_RSA_PADDING},
#endif
#ifdef SSL_OP_NETSCAPE_CA_DN_BUG
    {"NETSCAPE_CA_DN_BUG", SSL_OP_NETSCAPE_CA_DN_BUG},
#endif
#ifdef SSL_OP_NETSCAPE_CHALLENGE_BUG
    {"NETSCAPE_CHALLENGE_BUG", SSL_OP_NETSCAPE_CHALLENGE_BUG},
#endif
#ifdef SSL_OP_NETSCAPE_DEMO_CIPHER_CHANGE_BUG
    {"NETSCAPE_DEMO_CIPHER_CHANGE_BUG", SSL_OP_NETSCAPE_DEMO_CIPHER_CHANGE_BUG},
#endif
#ifdef SSL_OP_NETSCAPE_REUSE_CIPHER_CHANGE_BUG
    {"NETSCAPE_REUSE_CIPHER_CHANGE_BUG", SSL_OP_NETSCAPE_REUSE_CIPHER_CHANGE_BUG},
#endif
#ifdef SSL_OP_NO_ANTI_REPLAY
    {"NO_ANTI_REPLAY", SSL_OP_NO_ANTI_REPLAY},
#endif
#ifdef SSL_OP_NO_COMPRESSION
    {"NO_COMPRESSION", SSL_OP_NO_COMPRESSION},
#endif
#ifdef SSL_OP_NO_DTLS_MASK
    {"NO_DTLS_MASK", SSL_OP_NO_DTLS_MASK},
#endif
#ifdef SSL_OP_NO_DTLSv1
    {"NO_DTLSv1", SSL_OP_NO_DTLSv1},
#endif
#ifdef SSL_OP_NO_DTLSv1_2
    {"NO_DTLSv1_2", SSL_OP_NO_DTLSv1_2},
#endif
#ifdef SSL_OP_NO_ENCRYPT_THEN_MAC
    {"NO_ENCRYPT_THEN_MAC", SSL_OP_NO_ENCRYPT_THEN_MAC},
#endif
#ifdef SSL_OP_NO_EXTENDED_MASTER_SECRET
    {"NO_EXTENDED_MASTER_SECRET", SSL_OP_NO_EXTENDED_MASTER_SECRET},
#endif
#ifdef SSL_OP_NO_QUERY_MTU
    {"NO_QUERY_MTU", SSL_OP_NO_QUERY_MTU},
#endif
#ifdef SSL_OP_NO_RENEGOTIATION
    {"NO_RENEGOTIATION", SSL_OP_NO_RENEGOTIATION},
#endif
#ifdef SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION
    {"NO_SESSION_RESUMPTION_ON_RENEGOTIATION", SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION},
#endif
#ifdef SSL_OP_NO_SSL_MASK
    {"NO_SSL_MASK", SSL_OP_NO_SSL_MASK},
#endif
#ifdef SSL_OP_NO_SSLv2
    {"NO_SSLv2", SSL_OP_NO_SSLv2},
#endif
#ifdef SSL_OP_NO_SSLv3
    {"NO_SSLv3", SSL_OP_NO_SSLv3},
#endif
#ifdef SSL_OP_NO_TICKET
    {"NO_TICKET", SSL_OP_NO_TICKET},
#endif
#ifdef SSL_OP_NO_TLSv1
    {"NO_TLSv1", SSL_OP_NO_TLSv1},
#endif
#ifdef SSL_OP_NO_TLSv1_1
    {"NO_TLSv1_1", SSL_OP_NO_TLSv1_1},
#endif
#ifdef SSL_OP_NO_TLSv1_2
    {"NO_TLSv1_2", SSL_OP_NO_TLSv1_2},
#endif
#ifdef SSL_OP_NO_TLSv1_3
    {"NO_TLSv1_3", SSL_OP_NO_TLSv1_3},
#endif
#ifdef SSL_OP_PKCS1_CHECK_1
    {"PKCS1_CHECK_1", SSL_OP_PKCS1_CHECK_1},
#endif
#ifdef SSL_OP_PKCS1_CHECK_2
    {"PKCS1_CHECK_2", SSL_OP_PKCS1_CHECK_2},
#endif
#ifdef SSL_OP_PRIORITIZE_CHACHA
    {"PRIORITIZE_CHACHA", SSL_OP_PRIORITIZE_CHACHA},
#endif
#ifdef SSL_OP_SAFARI_ECDHE_ECDSA_BUG
    {"SAFARI_ECDHE_ECDSA_BUG", SSL_OP_SAFARI_ECDHE_ECDSA_BUG},
#endif
#ifdef SSL_OP_SINGLE_DH_USE
    {"SINGLE_DH_USE", SSL_OP_SINGLE_DH_USE},
#endif
#ifdef SSL_OP_SINGLE_ECDH_USE
    {"SINGLE_ECDH_USE", SSL_OP_SINGLE_ECDH_USE},
#endif
#ifdef SSL_OP_SSLEAY_080_CLIENT_DH_BUG
    {"SSLEAY_080_CLIENT_DH_BUG", SSL_OP_SSLEAY_080_CLIENT_DH_BUG},
#endif
#ifdef SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG
    {"SSLREF2_REUSE_CERT_TYPE_BUG", SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG},
#endif
#ifdef SSL_OP_TLS_BLOCK_PADDING_BUG
    {"TLS_BLOCK_PADDING_BUG", SSL_OP_TLS_BLOCK_PADDING_BUG},
#endif
#ifdef SSL_OP_TLS_D5_BUG
    {"TLS_D5_BUG", SSL_OP_TLS_D5_BUG},
#endif
#ifdef SSL_OP_TLSEXT_PADDING
    {"TLSEXT_PADDING", SSL_OP_TLSEXT_PADDING},
#endif
#ifdef SSL_OP_TLSEXT_PADDING_SUPER
    {"TLSEXT_PADDING_SUPER", SSL_OP_TLSEXT_PADDING_SUPER},
#endif
#ifdef SSL_OP_TLS_ROLLBACK_BUG
    {"TLS_ROLLBACK_BUG", SSL_OP_TLS_ROLLBACK_BUG},
#endif
    {NULL, 0}
};

NOEXPORT uint64_t parse_ssl_option(char *);
NOEXPORT void print_ssl_options(void);

NOEXPORT SOCK_OPT *socket_options_init(void);
NOEXPORT void socket_option_set_int(SOCK_OPT *, const char *, int, int);
NOEXPORT SOCK_OPT *socket_options_dup(SOCK_OPT *);
NOEXPORT void socket_options_free(SOCK_OPT *);
NOEXPORT int socket_options_print(void);
NOEXPORT char *socket_option_text(VAL_TYPE, OPT_UNION *);
NOEXPORT int socket_option_parse(SOCK_OPT *, char *);

#ifndef OPENSSL_NO_OCSP
NOEXPORT unsigned long parse_ocsp_flag(char *);
#endif /* !defined(OPENSSL_NO_OCSP) */

#ifndef OPENSSL_NO_ENGINE
NOEXPORT void engine_reset_list(void);
NOEXPORT const char *engine_auto(void);
NOEXPORT const char *engine_open(const char *);
NOEXPORT const char *engine_ctrl(const char *, const char *);
NOEXPORT const char *engine_default(const char *);
NOEXPORT const char *engine_init(void);
NOEXPORT ENGINE *engine_get_by_id(const char *);
NOEXPORT ENGINE *engine_get_by_num(const int);
#endif /* !defined(OPENSSL_NO_ENGINE) */

NOEXPORT const char *include_config(char *, SERVICE_OPTIONS **);

NOEXPORT void print_syntax(void);

NOEXPORT void name_list_append(NAME_LIST **, char *);
NOEXPORT void name_list_dup(NAME_LIST **, NAME_LIST *);
NOEXPORT void name_list_free(NAME_LIST *);
#ifndef USE_WIN32
NOEXPORT char **arg_alloc(char *);
NOEXPORT char **arg_dup(char **);
NOEXPORT void arg_free(char **arg);
#endif

static char *configuration_file=NULL;

GLOBAL_OPTIONS global_options;
SERVICE_OPTIONS service_options;
unsigned number_of_sections=0;

static GLOBAL_OPTIONS new_global_options;
static SERVICE_OPTIONS new_service_options;

static const char *option_not_found=
    "Specified option name is not valid here";

static const char *stunnel_cipher_list=
    "HIGH:!aNULL:!SSLv2:!DH:!kDHEPSK";

#ifndef OPENSSL_NO_TLS1_3
static const char *stunnel_ciphersuites=
    "TLS_AES_256_GCM_SHA384:TLS_AES_128_GCM_SHA256:TLS_CHACHA20_POLY1305_SHA256";
#endif /* TLS 1.3 */

/**************************************** parse commandline parameters */

/* return values:
   0 - configuration accepted
   1 - error
   2 - information printed
*/

int options_cmdline(char *arg1, char *arg2) {
    const char *name;
    CONF_TYPE type;

#ifdef USE_WIN32
    (void)arg2; /* squash the unused parameter warning */
#endif
    if(!arg1) {
        name=
#ifdef CONFDIR
            CONFDIR
#ifdef USE_WIN32
            "\\"
#else
            "/"
#endif
#endif
            "stunnel.conf";
        type=CONF_FILE;
    } else if(!strcasecmp(arg1, "-help")) {
        parse_global_option(CMD_PRINT_HELP, NULL, NULL, NULL);
        parse_service_option(CMD_PRINT_HELP, NULL, NULL, NULL);
        log_flush(LOG_MODE_INFO);
        return 2;
    } else if(!strcasecmp(arg1, "-version")) {
        parse_global_option(CMD_PRINT_DEFAULTS, NULL, NULL, NULL);
        parse_service_option(CMD_PRINT_DEFAULTS, NULL, NULL, NULL);
        log_flush(LOG_MODE_INFO);
        return 2;
    } else if(!strcasecmp(arg1, "-sockets")) {
        socket_options_print();
        log_flush(LOG_MODE_INFO);
        return 2;
    } else if(!strcasecmp(arg1, "-options")) {
        print_ssl_options();
        log_flush(LOG_MODE_INFO);
        return 2;
    } else
#ifndef USE_WIN32
    if(!strcasecmp(arg1, "-fd")) {
        if(!arg2) {
            s_log(LOG_ERR, "No file descriptor specified");
            print_syntax();
            return 1;
        }
        name=arg2;
        type=CONF_FD;
    } else
#endif
    {
        name=arg1;
        type=CONF_FILE;
    }

    if(type==CONF_FILE) {
#ifdef HAVE_REALPATH
        char *buffer=NULL, *real_path;
#ifdef MAXPATHLEN
        /* a workaround for pre-POSIX.1-2008 4.4BSD and Solaris */
        buffer=malloc(MAXPATHLEN);
#endif
        real_path=realpath(name, buffer);
        if(!real_path) {
            free(buffer);
            s_log(LOG_ERR, "Invalid configuration file name \"%s\"", name);
            ioerror("realpath");
            return 1;
        }
        configuration_file=str_dup(real_path);
        free(real_path);
#else
        configuration_file=str_dup(name);
#endif
#ifndef USE_WIN32
    } else if(type==CONF_FD) {
        configuration_file=str_dup(name);
#endif
    }
    return options_parse(type);
}

/**************************************** parse configuration file */

int options_parse(CONF_TYPE type) {
    SERVICE_OPTIONS *section;

    options_defaults();
    section=&new_service_options;
    /* options_file() is a recursive function, so the last section of the
     * configuration file section needs to be initialized separately */
    if(options_file(configuration_file, type, &section) ||
            init_section(1, &section)) {
        s_log(LOG_ERR, "Configuration failed");
        options_free(0); /* free the new options */
        return 1;
    }
    s_log(LOG_NOTICE, "Configuration successful");
    return 0;
}

NOEXPORT int options_file(char *path, CONF_TYPE type,
        SERVICE_OPTIONS **section_ptr) {
    DISK_FILE *df;
    char line_text[CONFLINELEN];
    const char *errstr;
    char config_line[CONFLINELEN], *config_opt, *config_arg;
    int i, line_number=0;
    const u_char utf8_bom[] = {0xef, 0xbb, 0xbf};
#ifndef USE_WIN32
    int fd;
    char *tmp_str;
#endif

    s_log(LOG_NOTICE, "Reading configuration from %s %s",
        type==CONF_FD ? "descriptor" : "file", path);
#ifndef USE_WIN32
    if(type==CONF_FD) { /* file descriptor */
        fd=(int)strtol(path, &tmp_str, 10);
        if(tmp_str==path || *tmp_str) { /* not a number */
            s_log(LOG_ERR, "Invalid file descriptor number");
            print_syntax();
            return 1;
        }
        df=file_fdopen(fd);
    } else
#endif
        df=file_open(path, FILE_MODE_READ);
    if(!df) {
        s_log(LOG_ERR, "Cannot open configuration file");
        if(type!=CONF_RELOAD)
            print_syntax();
        return 1;
    }

    while(file_getline(df, line_text, CONFLINELEN)>=0) {
        memcpy(config_line, line_text, CONFLINELEN);
        ++line_number;
        config_opt=config_line;
        if(line_number==1) {
            if(!memcmp(config_opt, utf8_bom, sizeof utf8_bom)) {
                s_log(LOG_NOTICE, "UTF-8 byte order mark detected");
                config_opt+=sizeof utf8_bom;
            } else {
                s_log(LOG_NOTICE, "UTF-8 byte order mark not detected");
            }
        }

        while(isspace((unsigned char)*config_opt))
            ++config_opt; /* remove initial whitespaces */
        for(i=(int)strlen(config_opt)-1; i>=0 && isspace((unsigned char)config_opt[i]); --i)
            config_opt[i]='\0'; /* remove trailing whitespaces */
        if(config_opt[0]=='\0' || config_opt[0]=='#' || config_opt[0]==';') /* empty or comment */
            continue;

        if(config_opt[0]=='[' && config_opt[strlen(config_opt)-1]==']') { /* new section */
            if(init_section(0, section_ptr)) {
                file_close(df);
                return 1;
            }

            /* append a new SERVICE_OPTIONS structure to the list */
            {
                SERVICE_OPTIONS *new_section;
                new_section=str_alloc_detached(sizeof(SERVICE_OPTIONS));
                new_section->next=NULL;
                (*section_ptr)->next=new_section;
                *section_ptr=new_section;
            }

            /* initialize the newly allocated section */
            ++config_opt;
            config_opt[strlen(config_opt)-1]='\0';
            (*section_ptr)->servname=str_dup_detached(config_opt);
            (*section_ptr)->session=NULL;
            parse_service_option(CMD_SET_COPY, section_ptr, NULL, NULL);
            continue;
        }

        config_arg=strchr(config_line, '=');
        if(!config_arg) {
            s_log(LOG_ERR, "%s:%d: \"%s\": No '=' found",
                path, line_number, line_text);
            file_close(df);
            return 1;
        }
        *config_arg++='\0'; /* split into option name and argument value */
        for(i=(int)strlen(config_opt)-1; i>=0 && isspace((unsigned char)config_opt[i]); --i)
            config_opt[i]='\0'; /* remove trailing whitespaces */
        while(isspace((unsigned char)*config_arg))
            ++config_arg; /* remove initial whitespaces */

        errstr=option_not_found;
        /* try global options first (e.g. for 'debug') */
        if(!new_service_options.next)
            errstr=parse_global_option(CMD_SET_VALUE, &new_global_options, config_opt, config_arg);
        if(errstr==option_not_found)
            errstr=parse_service_option(CMD_SET_VALUE, section_ptr, config_opt, config_arg);
        if(errstr) {
            s_log(LOG_ERR, "%s:%d: \"%s\": %s",
                path, line_number, line_text, errstr);
            file_close(df);
            return 1;
        }
    }
    file_close(df);
    return 0;
}

NOEXPORT int init_section(int eof, SERVICE_OPTIONS **section_ptr) {
    const char *errstr;

#ifndef USE_WIN32
    (*section_ptr)->option.log_stderr=new_global_options.option.log_stderr;
#endif /* USE_WIN32 */

    if(*section_ptr==&new_service_options) {
        /* end of global options or inetd mode -> initialize globals */
        errstr=parse_global_option(CMD_INITIALIZE, &new_global_options, NULL, NULL);
        if(errstr) {
            s_log(LOG_ERR, "Global options: %s", errstr);
            return 1;
        }
    }

    if(*section_ptr!=&new_service_options || eof) {
        /* end service section or inetd mode -> initialize service */
        errstr=parse_service_option(CMD_INITIALIZE, section_ptr, NULL, NULL);
        if(errstr) {
            if(*section_ptr==&new_service_options)
                s_log(LOG_ERR, "Inetd mode: %s", errstr);
            else
                s_log(LOG_ERR, "Service [%s]: %s",
                    (*section_ptr)->servname, errstr);
            return 1;
        }
    }
    return 0;
}

#ifdef USE_WIN32

int scandir(const char *dirp, struct dirent ***namelist,
        int (*filter)(const struct dirent *),
        int (*compar)(const struct dirent **, const struct dirent **)) {
    WIN32_FIND_DATA data;
    HANDLE h;
    unsigned num=0, allocated=0;
    LPTSTR path, pattern;
    char *name;
    DWORD saved_errno;

    (void)filter; /* squash the unused parameter warning */
    (void)compar; /* squash the unused parameter warning */
    path=str2tstr(dirp);
    pattern=str_tprintf(TEXT("%s\\*"), path);
    str_free(path);
    h=FindFirstFile(pattern, &data);
    saved_errno=GetLastError();
    str_free(pattern);
    SetLastError(saved_errno);
    if(h==INVALID_HANDLE_VALUE)
        return -1;
    *namelist=NULL;
    do {
        if(num>=allocated) {
            allocated+=16;
            *namelist=realloc(*namelist, allocated*sizeof(**namelist));
        }
        (*namelist)[num]=malloc(sizeof(struct dirent));
        if(!(*namelist)[num])
            return -1;
        name=tstr2str(data.cFileName);
        strncpy((*namelist)[num]->d_name, name, MAX_PATH-1);
        (*namelist)[num]->d_name[MAX_PATH-1]='\0';
        str_free(name);
        ++num;
    } while(FindNextFile(h, &data));
    FindClose(h);
    return (int)num;
}

int alphasort(const struct dirent **a, const struct dirent **b) {
    (void)a; /* squash the unused parameter warning */
    (void)b; /* squash the unused parameter warning */
    /* most Windows filesystem return sorted data */
    return 0;
}

#endif

void options_defaults() {
    SERVICE_OPTIONS *service;

    /* initialize globals *before* opening the config file */
    memset(&new_global_options, 0, sizeof(GLOBAL_OPTIONS));
    memset(&new_service_options, 0, sizeof(SERVICE_OPTIONS));
    new_service_options.next=NULL;

    parse_global_option(CMD_SET_DEFAULTS, &new_global_options, NULL, NULL);
    service=&new_service_options;
    parse_service_option(CMD_SET_DEFAULTS, &service, NULL, NULL);
}

void options_apply() { /* apply default/validated configuration */
    unsigned num=0;
    SERVICE_OPTIONS *section;

    CRYPTO_THREAD_write_lock(stunnel_locks[LOCK_SECTIONS]);

    memcpy(&global_options, &new_global_options, sizeof(GLOBAL_OPTIONS));
    memset(&new_global_options, 0, sizeof(GLOBAL_OPTIONS));

    /* service_options are used for inetd mode and to enumerate services */
    for(section=new_service_options.next; section; section=section->next)
        section->section_number=num++;
    memcpy(&service_options, &new_service_options, sizeof(SERVICE_OPTIONS));
    memset(&new_service_options, 0, sizeof(SERVICE_OPTIONS));
    number_of_sections=num;

    CRYPTO_THREAD_unlock(stunnel_locks[LOCK_SECTIONS]);
}

void options_free(int current) {
    GLOBAL_OPTIONS *global=current?&global_options:&new_global_options;
    SERVICE_OPTIONS *service=current?&service_options:&new_service_options;

    parse_global_option(CMD_FREE, global, NULL, NULL);

    CRYPTO_THREAD_write_lock(stunnel_locks[LOCK_SECTIONS]);
    while(service) {
        SERVICE_OPTIONS *tmp=service;
        service=service->next;
        tmp->next=NULL;
        service_free(tmp);
    }
    CRYPTO_THREAD_unlock(stunnel_locks[LOCK_SECTIONS]);
}

void service_up_ref(SERVICE_OPTIONS *section) {
#ifdef USE_OS_THREADS
    int ref;

    CRYPTO_atomic_add(&section->ref, 1, &ref, stunnel_locks[LOCK_REF]);
#else
    ++(section->ref);
#endif
}

void service_free(SERVICE_OPTIONS *section) {
    int ref;

#ifdef USE_OS_THREADS
    CRYPTO_atomic_add(&section->ref, -1, &ref, stunnel_locks[LOCK_REF]);
#else
    ref=--(section->ref);
#endif
    if(ref<0)
        fatal("Negative section reference counter");
    if(ref==0)
        parse_service_option(CMD_FREE, &section, NULL, NULL);
}

/**************************************** global options */

NOEXPORT const char *parse_global_option(CMD cmd, GLOBAL_OPTIONS *options, char *opt, char *arg) {
    void *tmp;

    if(cmd==CMD_PRINT_DEFAULTS || cmd==CMD_PRINT_HELP) {
        s_log(LOG_NOTICE, " ");
        s_log(LOG_NOTICE, "Global options:");
    }

    /* chroot */
#ifdef HAVE_CHROOT
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        options->chroot_dir=NULL;
        break;
    case CMD_SET_COPY: /* not used for global options */
        break;
    case CMD_FREE:
        tmp=options->chroot_dir;
        options->chroot_dir=NULL;
        str_free(tmp);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "chroot"))
            break;
        options->chroot_dir=str_dup(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = directory to chroot stunnel process", "chroot");
        break;
    }
#endif /* HAVE_CHROOT */

    /* compression */
#ifndef OPENSSL_NO_COMP
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        options->compression=COMP_NONE;
        break;
    case CMD_SET_COPY: /* not used for global options */
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "compression"))
            break;
#if OPENSSL_VERSION_NUMBER < 0x10100000L
        /* only allow compression with OpenSSL 0.9.8 or later
         * with OpenSSL #1468 zlib memory leak fixed */
        if(OpenSSL_version_num()<0x00908051L) /* 0.9.8e-beta1 */
            return "Compression unsupported due to a memory leak";
#endif /* OpenSSL version < 1.1.0 */
        if(!strcasecmp(arg, "deflate"))
            options->compression=COMP_DEFLATE;
        else if(!strcasecmp(arg, "zlib"))
            options->compression=COMP_ZLIB;
        else
            return "Specified compression type is not available";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = compression type",
            "compression");
        break;
    }
#endif /* !defined(OPENSSL_NO_COMP) */

    /* EGD */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
#ifdef EGD_SOCKET
        options->egd_sock=EGD_SOCKET;
#else
        options->egd_sock=NULL;
#endif
        break;
    case CMD_SET_COPY: /* not used for global options */
        break;
    case CMD_FREE:
        tmp=options->egd_sock;
        options->egd_sock=NULL;
        str_free(tmp);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "EGD"))
            break;
        options->egd_sock=str_dup(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
#ifdef EGD_SOCKET
        s_log(LOG_NOTICE, "%-22s = %s", "EGD", EGD_SOCKET);
#endif
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = path to Entropy Gathering Daemon socket", "EGD");
        break;
    }

#ifndef OPENSSL_NO_ENGINE

    /* engine */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        engine_reset_list();
        break;
    case CMD_SET_COPY: /* not used for global options */
        break;
    case CMD_FREE:
        /* FIXME: investigate if we can free it */
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "engine"))
            break;
        if(!strcasecmp(arg, "auto"))
            return engine_auto();
        else
            return engine_open(arg);
    case CMD_INITIALIZE:
        engine_init();
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = auto|engine_id",
            "engine");
        break;
    }

    /* engineCtrl */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        break;
    case CMD_SET_COPY: /* not used for global options */
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "engineCtrl"))
            break;
        {
            char *tmp_str=strchr(arg, ':');
            if(tmp_str)
                *tmp_str++='\0';
            return engine_ctrl(arg, tmp_str);
        }
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = cmd[:arg]",
            "engineCtrl");
        break;
    }

    /* engineDefault */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        break;
    case CMD_SET_COPY: /* not used for global options */
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "engineDefault"))
            break;
        return engine_default(arg);
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = TASK_LIST",
            "engineDefault");
        break;
    }

#endif /* !defined(OPENSSL_NO_ENGINE) */

    /* fips */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
#ifdef USE_FIPS
        options->option.fips=fips_default()?1:0;
#endif /* USE_FIPS */
        break;
    case CMD_SET_COPY: /* not used for global options */
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "fips"))
            break;
        if(!strcasecmp(arg, "yes")) {
#ifdef USE_FIPS
            options->option.fips=1;
#else
            return "FIPS support is not available";
#endif /* USE_FIPS */
        } else if(!strcasecmp(arg, "no")) {
#ifdef USE_FIPS
            if(fips_default())
                return "Failed to override system-wide FIPS mode";
            options->option.fips=0;
#endif /* USE_FIPS */
        } else {
            return "The argument needs to be either 'yes' or 'no'";
        }
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
#ifdef USE_FIPS
        if(fips_available())
            s_log(LOG_NOTICE, "%-22s = %s", "fips", fips_default()?"yes":"no");
#endif /* USE_FIPS */
        break;
    case CMD_PRINT_HELP:
        if(fips_available())
            s_log(LOG_NOTICE, "%-22s = yes|no FIPS 140-2 mode",
                "fips");
        break;
    }

    /* foreground */
#ifndef USE_WIN32
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        options->option.foreground=0;
        options->option.log_stderr=0;
        break;
    case CMD_SET_COPY: /* not used for global options */
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "foreground"))
            break;
        if(!strcasecmp(arg, "yes")) {
            options->option.foreground=1;
            options->option.log_stderr=1;
        } else if(!strcasecmp(arg, "quiet")) {
            options->option.foreground=1;
            options->option.log_stderr=0;
        } else if(!strcasecmp(arg, "no")) {
            options->option.foreground=0;
            options->option.log_stderr=0;
        } else
            return "The argument needs to be either 'yes', 'quiet' or 'no'";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = yes|quiet|no foreground mode (don't fork, log to stderr)",
            "foreground");
        break;
    }
#endif

#ifdef ICON_IMAGE

    /* iconActive */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        options->icon[ICON_ACTIVE]=load_icon_default(ICON_ACTIVE);
        break;
    case CMD_SET_COPY: /* not used for global options */
        break;
    case CMD_FREE:
        /* FIXME: investigate if we can free it */
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "iconActive"))
            break;
        options->icon[ICON_ACTIVE]=load_icon_file(arg);
        if(!options->icon[ICON_ACTIVE])
            return "Failed to load the specified icon";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = icon when connections are established", "iconActive");
        break;
    }

    /* iconError */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        options->icon[ICON_ERROR]=load_icon_default(ICON_ERROR);
        break;
    case CMD_SET_COPY: /* not used for global options */
        break;
    case CMD_FREE:
        /* FIXME: investigate if we can free it */
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "iconError"))
            break;
        options->icon[ICON_ERROR]=load_icon_file(arg);
        if(!options->icon[ICON_ERROR])
            return "Failed to load the specified icon";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = icon for invalid configuration file", "iconError");
        break;
    }

    /* iconIdle */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        options->icon[ICON_IDLE]=load_icon_default(ICON_IDLE);
        break;
    case CMD_SET_COPY: /* not used for global options */
        break;
    case CMD_FREE:
        /* FIXME: investigate if we can free it */
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "iconIdle"))
            break;
        options->icon[ICON_IDLE]=load_icon_file(arg);
        if(!options->icon[ICON_IDLE])
            return "Failed to load the specified icon";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = icon when no connections were established", "iconIdle");
        break;
    }

#endif /* ICON_IMAGE */

    /* log */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        options->log_file_mode=FILE_MODE_APPEND;
        break;
    case CMD_SET_COPY: /* not used for global options */
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "log"))
            break;
        if(!strcasecmp(arg, "append"))
            options->log_file_mode=FILE_MODE_APPEND;
        else if(!strcasecmp(arg, "overwrite"))
            options->log_file_mode=FILE_MODE_OVERWRITE;
        else
            return "The argument needs to be either 'append' or 'overwrite'";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = append|overwrite log file",
            "log");
        break;
    }

    /* output */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        options->output_file=NULL;
        break;
    case CMD_SET_COPY: /* not used for global options */
        break;
    case CMD_FREE:
        tmp=options->output_file;
        options->output_file=NULL;
        str_free(tmp);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "output"))
            break;
        options->output_file=str_dup(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
#ifndef USE_WIN32
        if(!options->option.foreground /* daemonize() used */ &&
                options->output_file /* log file enabled */ &&
                options->output_file[0]!='/' /* relative path */)
            return "Log file must include full path name";
#endif
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = file to append log messages", "output");
        break;
    }

    /* pid */
#ifndef USE_WIN32
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        options->pidfile=NULL; /* do not create a pid file */
        break;
    case CMD_SET_COPY: /* not used for global options */
        break;
    case CMD_FREE:
        tmp=options->pidfile;
        options->pidfile=NULL;
        str_free(tmp);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "pid"))
            break;
        if(arg[0]) /* is argument not empty? */
            options->pidfile=str_dup(arg);
        else
            options->pidfile=NULL; /* empty -> do not create a pid file */
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(!options->option.foreground /* daemonize() used */ &&
                options->pidfile /* pid file enabled */ &&
                options->pidfile[0]!='/' /* relative path */)
            return "Pid file must include full path name";
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = pid file", "pid");
        break;
    }
#endif

    /* RNDbytes */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        options->random_bytes=RANDOM_BYTES;
        break;
    case CMD_SET_COPY: /* not used for global options */
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "RNDbytes"))
            break;
        {
            char *tmp_str;
            options->random_bytes=(long)strtol(arg, &tmp_str, 10);
            if(tmp_str==arg || *tmp_str) /* not a number */
                return "Illegal number of bytes to read from random seed files";
        }
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        s_log(LOG_NOTICE, "%-22s = %d", "RNDbytes", RANDOM_BYTES);
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = bytes to read from random seed files", "RNDbytes");
        break;
    }

    /* RNDfile */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
#ifdef RANDOM_FILE
        options->rand_file=str_dup(RANDOM_FILE);
#else
        options->rand_file=NULL;
#endif
        break;
    case CMD_SET_COPY: /* not used for global options */
        break;
    case CMD_FREE:
        tmp=options->rand_file;
        options->rand_file=NULL;
        str_free(tmp);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "RNDfile"))
            break;
        options->rand_file=str_dup(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
#ifdef RANDOM_FILE
        s_log(LOG_NOTICE, "%-22s = %s", "RNDfile", RANDOM_FILE);
#endif
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = path to file with random seed data", "RNDfile");
        break;
    }

    /* RNDoverwrite */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        options->option.rand_write=1;
        break;
    case CMD_SET_COPY: /* not used for global options */
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "RNDoverwrite"))
            break;
        if(!strcasecmp(arg, "yes"))
            options->option.rand_write=1;
        else if(!strcasecmp(arg, "no"))
            options->option.rand_write=0;
        else
            return "The argument needs to be either 'yes' or 'no'";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        s_log(LOG_NOTICE, "%-22s = yes", "RNDoverwrite");
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = yes|no overwrite seed datafiles with new random data",
            "RNDoverwrite");
        break;
    }

    /* syslog */
#ifndef USE_WIN32
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        options->option.log_syslog=1;
        break;
    case CMD_SET_COPY: /* not used for global options */
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "syslog"))
            break;
        if(!strcasecmp(arg, "yes"))
            options->option.log_syslog=1;
        else if(!strcasecmp(arg, "no"))
            options->option.log_syslog=0;
        else
            return "The argument needs to be either 'yes' or 'no'";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = yes|no send logging messages to syslog",
            "syslog");
        break;
    }
#endif

    /* taskbar */
#ifdef USE_WIN32
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        options->option.taskbar=1;
        break;
    case CMD_SET_COPY: /* not used for global options */
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "taskbar"))
            break;
        if(!strcasecmp(arg, "yes"))
            options->option.taskbar=1;
        else if(!strcasecmp(arg, "no"))
            options->option.taskbar=0;
        else
            return "The argument needs to be either 'yes' or 'no'";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        s_log(LOG_NOTICE, "%-22s = yes", "taskbar");
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = yes|no enable the taskbar icon", "taskbar");
        break;
    }
#endif

    /* final checks */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        break;
    case CMD_SET_COPY:
        break;
    case CMD_FREE:
        memset(options, 0, sizeof(GLOBAL_OPTIONS));
        break;
    case CMD_SET_VALUE:
        return option_not_found;
    case CMD_INITIALIZE:
        /* FIPS needs to be initialized as early as possible */
        if(ssl_configure(options)) /* configure global TLS settings */
            return "Failed to initialize TLS";
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        break;
    }
    return NULL; /* OK */
}

/**************************************** service-level options */

NOEXPORT const char *parse_service_option(CMD cmd, SERVICE_OPTIONS **section_ptr,
        char *opt, char *arg) {
    SERVICE_OPTIONS *section;
    int endpoints=0;
#ifndef USE_WIN32
    struct group *gr;
    struct passwd *pw;
#endif

    section=section_ptr ? *section_ptr : NULL;

    if(cmd==CMD_SET_DEFAULTS || cmd==CMD_SET_COPY) {
        section->ref=1;
        if(section==&service_options)
            s_log(LOG_ERR, "INTERNAL ERROR: Initializing deployed section defaults");
        else if(section==&new_service_options)
            s_log(LOG_INFO, "Initializing inetd mode configuration");
        else
            s_log(LOG_INFO, "Initializing service [%s]", section->servname);
    } else if(cmd==CMD_FREE) {
        if(section==&service_options)
            s_log(LOG_DEBUG, "Deallocating deployed section defaults");
        else if(section==&new_service_options)
            s_log(LOG_DEBUG, "Deallocating temporary section defaults");
        else
            s_log(LOG_DEBUG, "Deallocating section [%s]", section->servname);
    } else if(cmd==CMD_PRINT_DEFAULTS || cmd==CMD_PRINT_HELP) {
        s_log(LOG_NOTICE, " ");
        s_log(LOG_NOTICE, "Service-level options:");
    }

    /* accept */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        addrlist_clear(&section->local_addr, 1);
        section->local_fd=NULL;
        break;
    case CMD_SET_COPY:
        addrlist_clear(&section->local_addr, 1);
        section->local_fd=NULL;
        name_list_dup(&section->local_addr.names,
            new_service_options.local_addr.names);
        break;
    case CMD_FREE:
        name_list_free(section->local_addr.names);
        str_free(section->local_addr.addr);
        str_free(section->local_fd);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "accept"))
            break;
        section->option.accept=1;
        name_list_append(&section->local_addr.names, arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(section->local_addr.names) {
            unsigned i;
            if(!addrlist_resolve(&section->local_addr))
                return "Cannot resolve accept target";
            section->local_fd=str_alloc_detached(section->local_addr.num*sizeof(SOCKET));
            for(i=0; i<section->local_addr.num; ++i)
                section->local_fd[i]=INVALID_SOCKET;
            ++endpoints;
        }
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = [host:]port accept connections on specified host:port",
            "accept");
        break;
    }

    /* CApath */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
#if 0
        section->ca_dir=(char *)X509_get_default_cert_dir();
#endif
        section->ca_dir=NULL;
        break;
    case CMD_SET_COPY:
        section->ca_dir=str_dup_detached(new_service_options.ca_dir);
        break;
    case CMD_FREE:
        str_free(section->ca_dir);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "CApath"))
            break;
        str_free(section->ca_dir);
        if(arg[0]) /* not empty */
            section->ca_dir=str_dup_detached(arg);
        else
            section->ca_dir=NULL;
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
#if 0
        s_log(LOG_NOTICE, "%-22s = %s", "CApath",
            section->ca_dir ? section->ca_dir : "(none)");
#endif
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = CA certificate directory for 'verify' option",
            "CApath");
        break;
    }

    /* CAfile */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
#if 0
        section->ca_file=(char *)X509_get_default_certfile();
#endif
        section->ca_file=NULL;
        break;
    case CMD_SET_COPY:
        section->ca_file=str_dup_detached(new_service_options.ca_file);
        break;
    case CMD_FREE:
        str_free(section->ca_file);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "CAfile"))
            break;
        str_free(section->ca_file);
        if(arg[0]) /* not empty */
            section->ca_file=str_dup_detached(arg);
        else
            section->ca_file=NULL;
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
#if 0
        s_log(LOG_NOTICE, "%-22s = %s", "CAfile",
            section->ca_file ? section->ca_file : "(none)");
#endif
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = CA certificate file for 'verify' option",
            "CAfile");
        break;
    }

    /* cert */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->cert=NULL;
        break;
    case CMD_SET_COPY:
        section->cert=str_dup_detached(new_service_options.cert);
        break;
    case CMD_FREE:
        str_free(section->cert);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "cert"))
            break;
        str_free(section->cert);
        section->cert=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
#ifndef OPENSSL_NO_PSK
        if(section->psk_keys)
            break;
#endif /* !defined(OPENSSL_NO_PSK) */
#ifndef OPENSSL_NO_ENGINE
        if(section->engine)
            break;
#endif /* !defined(OPENSSL_NO_ENGINE) */
        if(!section->option.client && !section->cert)
            return "TLS server needs a certificate";
        break;
    case CMD_PRINT_DEFAULTS:
        break; /* no default certificate */
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = certificate chain", "cert");
        break;
    }

#if OPENSSL_VERSION_NUMBER>=0x10002000L

    /* checkEmail */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->check_email=NULL;
        break;
    case CMD_SET_COPY:
        name_list_dup(&section->check_email,
            new_service_options.check_email);
        break;
    case CMD_FREE:
        name_list_free(section->check_email);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "checkEmail"))
            break;
        name_list_append(&section->check_email, arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(section->check_email && !section->option.verify_chain && !section->option.verify_peer)
            return "Either \"verifyChain\" or \"verifyPeer\" has to be enabled";
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = peer certificate email address",
            "checkEmail");
        break;
    }

    /* checkHost */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->check_host=NULL;
        break;
    case CMD_SET_COPY:
        name_list_dup(&section->check_host,
            new_service_options.check_host);
        break;
    case CMD_FREE:
        name_list_free(section->check_host);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "checkHost"))
            break;
        name_list_append(&section->check_host, arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(section->check_host && !section->option.verify_chain && !section->option.verify_peer)
            return "Either \"verifyChain\" or \"verifyPeer\" has to be enabled";
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = peer certificate host name pattern",
            "checkHost");
        break;
    }

    /* checkIP */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->check_ip=NULL;
        break;
    case CMD_SET_COPY:
        name_list_dup(&section->check_ip,
            new_service_options.check_ip);
        break;
    case CMD_FREE:
        name_list_free(section->check_ip);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "checkIP"))
            break;
        name_list_append(&section->check_ip, arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(section->check_ip && !section->option.verify_chain && !section->option.verify_peer)
            return "Either \"verifyChain\" or \"verifyPeer\" has to be enabled";
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = peer certificate IP address",
            "checkIP");
        break;
    }

#endif /* OPENSSL_VERSION_NUMBER>=0x10002000L */

    /* ciphers */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->cipher_list=NULL;
        break;
    case CMD_SET_COPY:
        section->cipher_list=str_dup_detached(new_service_options.cipher_list);
        break;
    case CMD_FREE:
        str_free(section->cipher_list);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "ciphers"))
            break;
        str_free(section->cipher_list);
        section->cipher_list=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(!section->cipher_list) {
            /* this is only executed for global options, because
             * section->cipher_list is no longer NULL in sections */
#ifdef USE_FIPS
            if(new_global_options.option.fips)
                section->cipher_list=str_dup_detached("FIPS");
            else
#endif /* USE_FIPS */
                section->cipher_list=str_dup_detached(stunnel_cipher_list);
        }
        break;
    case CMD_PRINT_DEFAULTS:
        if(fips_available()) {
            s_log(LOG_NOTICE, "%-22s = %s %s", "ciphers",
                "FIPS", "(with \"fips = yes\")");
            s_log(LOG_NOTICE, "%-22s = %s %s", "ciphers",
                stunnel_cipher_list, "(with \"fips = no\")");
        } else {
            s_log(LOG_NOTICE, "%-22s = %s", "ciphers", stunnel_cipher_list);
        }
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = permitted ciphers for TLS 1.2 or older", "ciphers");
        break;
    }

#ifndef OPENSSL_NO_TLS1_3
    /* ciphersuites */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->ciphersuites=NULL;
        break;
    case CMD_SET_COPY:
        section->ciphersuites=str_dup_detached(new_service_options.ciphersuites);
        break;
    case CMD_FREE:
        str_free(section->ciphersuites);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "ciphersuites"))
            break;
        str_free(section->ciphersuites);
        section->ciphersuites=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(!section->ciphersuites) {
            /* this is only executed for global options, because
             * section->ciphersuites is no longer NULL in sections */
            section->ciphersuites=str_dup_detached(stunnel_ciphersuites);
        }
        break;
    case CMD_PRINT_DEFAULTS:
        s_log(LOG_NOTICE, "%-22s = %s %s", "ciphersuites", stunnel_ciphersuites, "(with TLSv1.3)");
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = permitted ciphersuites for TLS 1.3", "ciphersuites");
        break;
    }
#endif /* TLS 1.3 */

    /* client */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.client=0;
        break;
    case CMD_SET_COPY:
        section->option.client=new_service_options.option.client;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "client"))
            break;
        if(!strcasecmp(arg, "yes"))
            section->option.client=1;
        else if(!strcasecmp(arg, "no"))
            section->option.client=0;
        else
            return "The argument needs to be either 'yes' or 'no'";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = yes|no client mode (remote service uses TLS)",
            "client");
        break;
    }

#if OPENSSL_VERSION_NUMBER>=0x10002000L

    /* config */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->config=NULL;
        break;
    case CMD_SET_COPY:
        name_list_dup(&section->config, new_service_options.config);
        break;
    case CMD_FREE:
        name_list_free(section->config);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "config"))
            break;
        name_list_append(&section->config, arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = command[:parameter] to execute",
            "config");
        break;
    }

#endif /* OPENSSL_VERSION_NUMBER>=0x10002000L */

    /* connect */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        addrlist_clear(&section->connect_addr, 0);
        section->connect_session=NULL;
        break;
    case CMD_SET_COPY:
        addrlist_clear(&section->connect_addr, 0);
        section->connect_session=NULL;
        name_list_dup(&section->connect_addr.names,
            new_service_options.connect_addr.names);
        break;
    case CMD_FREE:
        name_list_free(section->connect_addr.names);
        str_free(section->connect_addr.addr);
        str_free(section->connect_session);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "connect"))
            break;
        name_list_append(&section->connect_addr.names, arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(section->connect_addr.names) {
            if(!section->option.delayed_lookup &&
                    !addrlist_resolve(&section->connect_addr)) {
                s_log(LOG_INFO,
                    "Cannot resolve connect target - delaying DNS lookup");
                section->connect_addr.num=0;
                section->redirect_addr.num=0;
                section->option.delayed_lookup=1;
            }
            if(section->option.client)
                section->connect_session=
                    str_alloc_detached(section->connect_addr.num*sizeof(SSL_SESSION *));
            ++endpoints;
        }
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = [host:]port to connect",
            "connect");
        break;
    }

    /* CRLpath */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->crl_dir=NULL;
        break;
    case CMD_SET_COPY:
        section->crl_dir=str_dup_detached(new_service_options.crl_dir);
        break;
    case CMD_FREE:
        str_free(section->crl_dir);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "CRLpath"))
            break;
        str_free(section->crl_dir);
        if(arg[0]) /* not empty */
            section->crl_dir=str_dup_detached(arg);
        else
            section->crl_dir=NULL;
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = CRL directory", "CRLpath");
        break;
    }

    /* CRLfile */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->crl_file=NULL;
        break;
    case CMD_SET_COPY:
        section->crl_file=str_dup_detached(new_service_options.crl_file);
        break;
    case CMD_FREE:
        str_free(section->crl_file);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "CRLfile"))
            break;
        str_free(section->crl_file);
        if(arg[0]) /* not empty */
            section->crl_file=str_dup_detached(arg);
        else
            section->crl_file=NULL;
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = CRL file", "CRLfile");
        break;
    }

#ifndef OPENSSL_NO_ECDH

    /* curves */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->curves=str_dup_detached(DEFAULT_CURVES);
        break;
    case CMD_SET_COPY:
        section->curves=str_dup_detached(new_service_options.curves);
        break;
    case CMD_FREE:
        str_free(section->curves);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "curves") && strcasecmp(opt, "curve"))
            break;
        str_free(section->curves);
        section->curves=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        s_log(LOG_NOTICE, "%-22s = %s", "curves", DEFAULT_CURVES);
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = ECDH curve names", "curves");
        break;
    }

#endif /* !defined(OPENSSL_NO_ECDH) */

    /* debug */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->log_level=LOG_NOTICE;
#if !defined (USE_WIN32) && !defined (__vms)
        new_global_options.log_facility=LOG_DAEMON;
#endif
        break;
    case CMD_SET_COPY:
        section->log_level=new_service_options.log_level;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "debug"))
            break;
        return parse_debug_level(arg, section);
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
#if !defined (USE_WIN32) && !defined (__vms)
        s_log(LOG_NOTICE, "%-22s = %s", "debug", "daemon.notice");
#else
        s_log(LOG_NOTICE, "%-22s = %s", "debug", "notice");
#endif
        break;
    case CMD_PRINT_HELP:
#if !defined (USE_WIN32) && !defined (__vms)
        s_log(LOG_NOTICE, "%-22s = [facility].level (e.g. daemon.info)", "debug");
#else
        s_log(LOG_NOTICE, "%-22s = level (e.g. info)", "debug");
#endif
        break;
    }

    /* delay */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.delayed_lookup=0;
        break;
    case CMD_SET_COPY:
        section->option.delayed_lookup=new_service_options.option.delayed_lookup;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "delay"))
            break;
        if(!strcasecmp(arg, "yes"))
            section->option.delayed_lookup=1;
        else if(!strcasecmp(arg, "no"))
            section->option.delayed_lookup=0;
        else
            return "The argument needs to be either 'yes' or 'no'";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE,
            "%-22s = yes|no delay DNS lookup for 'connect' option",
            "delay");
        break;
    }

#ifndef OPENSSL_NO_ENGINE

    /* engineId */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        break;
    case CMD_SET_COPY:
        section->engine=new_service_options.engine;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "engineId"))
            break;
        section->engine=engine_get_by_id(arg);
        if(!section->engine)
            return "Engine ID not found";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = ID of engine to read the key from",
            "engineId");
        break;
    }

    /* engineNum */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        break;
    case CMD_SET_COPY:
        section->engine=new_service_options.engine;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "engineNum"))
            break;
        {
            char *tmp_str;
            int tmp_int=(int)strtol(arg, &tmp_str, 10);
            if(tmp_str==arg || *tmp_str) /* not a number */
                return "Illegal engine number";
            section->engine=engine_get_by_num(tmp_int-1);
        }
        if(!section->engine)
            return "Illegal engine number";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = number of engine to read the key from",
            "engineNum");
        break;
    }

#endif /* !defined(OPENSSL_NO_ENGINE) */

    /* exec */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->exec_name=NULL;
        break;
    case CMD_SET_COPY:
        section->exec_name=str_dup_detached(new_service_options.exec_name);
        break;
    case CMD_FREE:
        str_free(section->exec_name);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "exec"))
            break;
        str_free(section->exec_name);
        section->exec_name=str_dup_detached(arg);
#ifdef USE_WIN32
        section->exec_args=str_dup_detached(arg);
#else
        if(!section->exec_args) {
            section->exec_args=str_alloc_detached(2*sizeof(char *));
            section->exec_args[0]=str_dup_detached(section->exec_name);
            section->exec_args[1]=NULL; /* null-terminate */
        }
#endif
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(section->exec_name)
            ++endpoints;
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = file execute local inetd-type program",
            "exec");
        break;
    }

    /* execArgs */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->exec_args=NULL;
        break;
    case CMD_SET_COPY:
#ifdef USE_WIN32
        section->exec_args=str_dup_detached(new_service_options.exec_args);
#else
        section->exec_args=arg_dup(new_service_options.exec_args);
#endif
        break;
    case CMD_FREE:
#ifdef USE_WIN32
        str_free(section->exec_args);
#else
        arg_free(section->exec_args);
#endif
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "execArgs"))
            break;
#ifdef USE_WIN32
        str_free(section->exec_args);
        section->exec_args=str_dup_detached(arg);
#else
        arg_free(section->exec_args);
        section->exec_args=arg_alloc(arg);
#endif
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = arguments for 'exec' (including $0)",
            "execArgs");
        break;
    }

    /* failover */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->failover=FAILOVER_PRIO;
        section->rr=0;
        break;
    case CMD_SET_COPY:
        section->failover=new_service_options.failover;
        section->rr=new_service_options.rr;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "failover"))
            break;
        if(!strcasecmp(arg, "rr"))
            section->failover=FAILOVER_RR;
        else if(!strcasecmp(arg, "prio"))
            section->failover=FAILOVER_PRIO;
        else
            return "The argument needs to be either 'rr' or 'prio'";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(section->option.delayed_lookup)
            section->failover=FAILOVER_PRIO;
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = rr|prio failover strategy",
            "failover");
        break;
    }

    /* ident */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->username=NULL;
        break;
    case CMD_SET_COPY:
        section->username=str_dup_detached(new_service_options.username);
        break;
    case CMD_FREE:
        str_free(section->username);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "ident"))
            break;
        str_free(section->username);
        section->username=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = username for IDENT (RFC 1413) checking", "ident");
        break;
    }

    /* include */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        break;
    case CMD_SET_COPY:
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "include"))
            break;
        return include_config(arg, section_ptr);
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = directory with configuration file snippets",
            "include");
        break;
    }

    /* key */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->key=NULL;
        break;
    case CMD_SET_COPY:
        section->key=str_dup_detached(new_service_options.key);
        break;
    case CMD_FREE:
        str_free(section->key);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "key"))
            break;
        str_free(section->key);
        section->key=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(section->cert && !section->key)
            section->key=str_dup_detached(section->cert);
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = certificate private key", "key");
        break;
    }

    /* libwrap */
#ifdef USE_LIBWRAP
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.libwrap=0; /* disable libwrap by default */
        break;
    case CMD_SET_COPY:
        section->option.libwrap=new_service_options.option.libwrap;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "libwrap"))
            break;
        if(!strcasecmp(arg, "yes"))
            section->option.libwrap=1;
        else if(!strcasecmp(arg, "no"))
            section->option.libwrap=0;
        else
            return "The argument needs to be either 'yes' or 'no'";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = yes|no use /etc/hosts.allow and /etc/hosts.deny",
            "libwrap");
        break;
    }
#endif /* USE_LIBWRAP */

    /* local */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.local=0;
        break;
    case CMD_SET_COPY:
        section->option.local=new_service_options.option.local;
        memcpy(&section->source_addr, &new_service_options.source_addr,
            sizeof(SOCKADDR_UNION));
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "local"))
            break;
        if(!hostport2addr(&section->source_addr, arg, "0", 1))
            return "Failed to resolve local address";
        section->option.local=1;
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = IP address to be used as source for remote"
            " connections", "local");
        break;
    }

    /* logId */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->log_id=LOG_ID_SEQUENTIAL;
        break;
    case CMD_SET_COPY:
        section->log_id=new_service_options.log_id;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "logId"))
            break;
        if(!strcasecmp(arg, "sequential"))
            section->log_id=LOG_ID_SEQUENTIAL;
        else if(!strcasecmp(arg, "unique"))
            section->log_id=LOG_ID_UNIQUE;
        else if(!strcasecmp(arg, "thread"))
            section->log_id=LOG_ID_THREAD;
        else if(!strcasecmp(arg, "process"))
            section->log_id=LOG_ID_PROCESS;
        else
            return "Invalid connection identifier type";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        s_log(LOG_NOTICE, "%-22s = %s", "logId", "sequential");
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = connection identifier type",
            "logId");
        break;
    }

#ifndef OPENSSL_NO_OCSP

    /* OCSP */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->ocsp_url=NULL;
        break;
    case CMD_SET_COPY:
        section->ocsp_url=str_dup_detached(new_service_options.ocsp_url);
        break;
    case CMD_FREE:
        str_free(section->ocsp_url);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "ocsp"))
            break;
        str_free(section->ocsp_url);
        section->ocsp_url=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = OCSP responder URL", "OCSP");
        break;
    }

    /* OCSPaia */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.aia=0; /* disable AIA by default */
        break;
    case CMD_SET_COPY:
        section->option.aia=new_service_options.option.aia;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "OCSPaia"))
            break;
        if(!strcasecmp(arg, "yes"))
            section->option.aia=1;
        else if(!strcasecmp(arg, "no"))
            section->option.aia=0;
        else
            return "The argument needs to be either 'yes' or 'no'";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE,
            "%-22s = yes|no check the AIA responders from certificates",
            "OCSPaia");
        break;
    }

    /* OCSPflag */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->ocsp_flags=0;
        break;
    case CMD_SET_COPY:
        section->ocsp_flags=new_service_options.ocsp_flags;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "OCSPflag"))
            break;
        {
            unsigned long tmp_ulong=parse_ocsp_flag(arg);
            if(!tmp_ulong)
                return "Illegal OCSP flag";
            section->ocsp_flags|=tmp_ulong;
        }
        return NULL;
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = OCSP responder flags", "OCSPflag");
        break;
    }

    /* OCSPnonce */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.nonce=0; /* disable OCSP nonce by default */
        break;
    case CMD_SET_COPY:
        section->option.nonce=new_service_options.option.nonce;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "OCSPnonce"))
            break;
        if(!strcasecmp(arg, "yes"))
            section->option.nonce=1;
        else if(!strcasecmp(arg, "no"))
            section->option.nonce=0;
        else
            return "The argument needs to be either 'yes' or 'no'";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE,
            "%-22s = yes|no send and verify the OCSP nonce extension",
            "OCSPnonce");
        break;
    }

#endif /* !defined(OPENSSL_NO_OCSP) */

    /* options */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->ssl_options_set=0;
#if OPENSSL_VERSION_NUMBER>=0x009080dfL
        section->ssl_options_clear=0;
#endif /* OpenSSL 0.9.8m or later */
        break;
    case CMD_SET_COPY:
        section->ssl_options_set=new_service_options.ssl_options_set;
#if OPENSSL_VERSION_NUMBER>=0x009080dfL
        section->ssl_options_clear=new_service_options.ssl_options_clear;
#endif /* OpenSSL 0.9.8m or later */
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "options"))
            break;
#if OPENSSL_VERSION_NUMBER>=0x009080dfL
        if(*arg=='-') {
            uint64_t tmp=parse_ssl_option(arg+1);
            if(tmp==INVALID_SSL_OPTION)
                return "Illegal TLS option";
            section->ssl_options_clear|=tmp;
            return NULL; /* OK */
        }
#endif /* OpenSSL 0.9.8m or later */
        {
            uint64_t tmp=parse_ssl_option(arg);
            if(tmp==INVALID_SSL_OPTION)
                return "Illegal TLS option";
            section->ssl_options_set|=tmp;
        }
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        s_log(LOG_NOTICE, "%-22s = %s", "options", "NO_SSLv2");
        s_log(LOG_NOTICE, "%-22s = %s", "options", "NO_SSLv3");
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = TLS option to set/reset", "options");
        break;
    }

    /* protocol */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->protocol=NULL;
        break;
    case CMD_SET_COPY:
        section->protocol=str_dup_detached(new_service_options.protocol);
        break;
    case CMD_FREE:
        str_free(section->protocol);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "protocol"))
            break;
        str_free(section->protocol);
        section->protocol=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        /* PROTOCOL_CHECK also initializes:
           section->option.connect_before_ssl
           section->option.protocol_endpoint */
        {
            const char *tmp_str=protocol(NULL, section, PROTOCOL_CHECK);
            if(tmp_str)
                return tmp_str;
        }
        endpoints+=section->option.protocol_endpoint;
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = protocol to negotiate before TLS initialization",
            "protocol");
        s_log(LOG_NOTICE, "%25scurrently supported: cifs, connect, imap,", "");
        s_log(LOG_NOTICE, "%25s    nntp, pgsql, pop3, proxy, smtp, socks", "");
        break;
    }

    /* protocolAuthentication */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->protocol_authentication=str_dup_detached("basic");
        break;
    case CMD_SET_COPY:
        section->protocol_authentication=
            str_dup_detached(new_service_options.protocol_authentication);
        break;
    case CMD_FREE:
        str_free(section->protocol_authentication);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "protocolAuthentication"))
            break;
        str_free(section->protocol_authentication);
        section->protocol_authentication=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = authentication type for protocol negotiations",
            "protocolAuthentication");
        break;
    }

    /* protocolDomain */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->protocol_domain=NULL;
        break;
    case CMD_SET_COPY:
        section->protocol_domain=
            str_dup_detached(new_service_options.protocol_domain);
        break;
    case CMD_FREE:
        str_free(section->protocol_domain);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "protocolDomain"))
            break;
        str_free(section->protocol_domain);
        section->protocol_domain=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = domain for protocol negotiations",
            "protocolDomain");
        break;
    }

    /* protocolHeader */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->protocol_header=NULL;
        break;
    case CMD_SET_COPY:
        name_list_dup(&section->protocol_header,
            new_service_options.protocol_header);
        break;
    case CMD_FREE:
        name_list_free(section->protocol_header);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "protocolHeader"))
            break;
        name_list_append(&section->protocol_header, arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = custom header for protocol negotiations",
            "protocolHeader");
        break;
    }

    /* protocolHost */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->protocol_host=NULL;
        break;
    case CMD_SET_COPY:
        section->protocol_host=
            str_dup_detached(new_service_options.protocol_host);
        break;
    case CMD_FREE:
        str_free(section->protocol_host);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "protocolHost"))
            break;
        str_free(section->protocol_host);
        section->protocol_host=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = host:port for protocol negotiations",
            "protocolHost");
        break;
    }

    /* protocolPassword */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->protocol_password=NULL;
        break;
    case CMD_SET_COPY:
        section->protocol_password=
            str_dup_detached(new_service_options.protocol_password);
        break;
    case CMD_FREE:
        str_free(section->protocol_password);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "protocolPassword"))
            break;
        str_free(section->protocol_password);
        section->protocol_password=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = password for protocol negotiations",
            "protocolPassword");
        break;
    }

    /* protocolUsername */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->protocol_username=NULL;
        break;
    case CMD_SET_COPY:
        section->protocol_username=
            str_dup_detached(new_service_options.protocol_username);
        break;
    case CMD_FREE:
        str_free(section->protocol_username);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "protocolUsername"))
            break;
        str_free(section->protocol_username);
        section->protocol_username=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = username for protocol negotiations",
            "protocolUsername");
        break;
    }

#ifndef OPENSSL_NO_PSK

    /* PSKidentity */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->psk_identity=NULL;
        section->psk_selected=NULL;
        section->psk_sorted.val=NULL;
        section->psk_sorted.num=0;
        break;
    case CMD_SET_COPY:
        section->psk_identity=
            str_dup_detached(new_service_options.psk_identity);
        break;
    case CMD_FREE:
        str_free(section->psk_identity);
        str_free(section->psk_sorted.val);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "PSKidentity"))
            break;
        str_free(section->psk_identity);
        section->psk_identity=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(!section->psk_keys) /* PSK not configured */
            break;
        psk_sort(&section->psk_sorted, section->psk_keys);
        if(section->option.client) {
            if(section->psk_identity) {
                section->psk_selected=
                    psk_find(&section->psk_sorted, section->psk_identity);
                if(!section->psk_selected)
                    return "No key found for the specified PSK identity";
            } else { /* take the first specified identity as default */
                section->psk_selected=section->psk_keys;
            }
        } else {
            if(section->psk_identity)
                s_log(LOG_NOTICE,
                    "PSK identity is ignored in the server mode");
        }
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = identity for PSK authentication",
            "PSKidentity");
        break;
    }

    /* PSKsecrets */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->psk_keys=NULL;
        break;
    case CMD_SET_COPY:
        section->psk_keys=psk_dup(new_service_options.psk_keys);
        break;
    case CMD_FREE:
        psk_free(section->psk_keys);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "PSKsecrets"))
            break;
        section->psk_keys=psk_read(arg);
        if(!section->psk_keys)
            return "Failed to read PSK secrets";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = secrets for PSK authentication",
            "PSKsecrets");
        break;
    }

#endif /* !defined(OPENSSL_NO_PSK) */

    /* pty */
#ifndef USE_WIN32
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.pty=0;
        break;
    case CMD_SET_COPY:
        section->option.pty=new_service_options.option.pty;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "pty"))
            break;
        if(!strcasecmp(arg, "yes"))
            section->option.pty=1;
        else if(!strcasecmp(arg, "no"))
            section->option.pty=0;
        else
            return "The argument needs to be either 'yes' or 'no'";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = yes|no allocate pseudo terminal for 'exec' option",
            "pty");
        break;
    }
#endif

    /* redirect */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        addrlist_clear(&section->redirect_addr, 0);
        break;
    case CMD_SET_COPY:
        addrlist_clear(&section->redirect_addr, 0);
        name_list_dup(&section->redirect_addr.names,
            new_service_options.redirect_addr.names);
        break;
    case CMD_FREE:
        name_list_free(section->redirect_addr.names);
        str_free(section->redirect_addr.addr);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "redirect"))
            break;
        name_list_append(&section->redirect_addr.names, arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(section->redirect_addr.names) {
            if(section->option.client)
                return "\"redirect\" is unsupported in client sections";
            if(section->option.connect_before_ssl)
                return "\"redirect\" is incompatible with the specified protocol negotiation";
            if(!section->option.delayed_lookup &&
                    !addrlist_resolve(&section->redirect_addr)) {
                s_log(LOG_INFO,
                    "Cannot resolve redirect target - delaying DNS lookup");
                section->connect_addr.num=0;
                section->redirect_addr.num=0;
                section->option.delayed_lookup=1;
            }
            if(!section->option.verify_chain && !section->option.verify_peer)
                return "Either \"verifyChain\" or \"verifyPeer\" has to be enabled for \"redirect\" to work";
        }
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE,
            "%-22s = [host:]port to redirect on authentication failures",
            "redirect");
        break;
    }

    /* renegotiation */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.renegotiation=1;
        break;
    case CMD_SET_COPY:
        section->option.renegotiation=new_service_options.option.renegotiation;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "renegotiation"))
            break;
        if(!strcasecmp(arg, "yes"))
            section->option.renegotiation=1;
        else if(!strcasecmp(arg, "no"))
            section->option.renegotiation=0;
        else
            return "The argument needs to be either 'yes' or 'no'";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = yes|no support renegotiation",
              "renegotiation");
        break;
    }

    /* requireCert */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.require_cert=0;
        break;
    case CMD_SET_COPY:
        section->option.require_cert=new_service_options.option.require_cert;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "requireCert"))
            break;
        if(!strcasecmp(arg, "yes")) {
            section->option.request_cert=1;
            section->option.require_cert=1;
        } else if(!strcasecmp(arg, "no")) {
            section->option.require_cert=0;
        } else {
            return "The argument needs to be either 'yes' or 'no'";
        }
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = yes|no require client certificate",
            "requireCert");
        break;
    }

    /* reset */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.reset=1; /* enabled by default */
        break;
    case CMD_SET_COPY:
        section->option.reset=new_service_options.option.reset;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "reset"))
            break;
        if(!strcasecmp(arg, "yes"))
            section->option.reset=1;
        else if(!strcasecmp(arg, "no"))
            section->option.reset=0;
        else
            return "The argument needs to be either 'yes' or 'no'";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = yes|no send TCP RST on error",
            "reset");
        break;
    }

    /* retry */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.retry=0;
        break;
    case CMD_SET_COPY:
        section->option.retry=new_service_options.option.retry;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "retry"))
            break;
        if(!strcasecmp(arg, "yes"))
            section->option.retry=1;
        else if(!strcasecmp(arg, "no"))
            section->option.retry=0;
        else
            return "The argument needs to be either 'yes' or 'no'";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = yes|no retry connect+exec section",
            "retry");
        break;
    }

#if OPENSSL_VERSION_NUMBER>=0x10100000L
    /* securityLevel */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->security_level=-1;
        break;
    case CMD_SET_COPY:
        section->security_level=new_service_options.security_level;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "securityLevel"))
            break;
        {
            char *tmp_str;
            int tmp_int =(int)strtol(arg, &tmp_str, 10);
            if(tmp_str==arg || *tmp_str || tmp_int<0 || tmp_int>5) /* not a correct number */
                return "Illegal security level";
            section->security_level = tmp_int;
        }
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        s_log(LOG_NOTICE, "%-22s = %d", "securityLevel", DEFAULT_SECURITY_LEVEL);
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = set the security level", "securityLevel");
        break;
    }
#endif /* OpenSSL 1.1.0 or later */

#ifndef USE_WIN32
    /* service */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->servname=str_dup_detached("stunnel");
        break;
    case CMD_SET_COPY:
        /* servname is *not* copied from the global section */
        break;
    case CMD_FREE:
        /* deallocation is performed at the end CMD_FREE */
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "service"))
            break;
        str_free(section->servname);
        section->servname=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = service name", "service");
        break;
    }
#endif

#ifndef USE_WIN32
    /* setgid */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->gid=0;
        break;
    case CMD_SET_COPY:
        section->gid=new_service_options.gid;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "setgid"))
            break;
        gr=getgrnam(arg);
        if(gr) {
            section->gid=gr->gr_gid;
            return NULL; /* OK */
        }
        {
            char *tmp_str;
            section->gid=(gid_t)strtol(arg, &tmp_str, 10);
            if(tmp_str==arg || *tmp_str) /* not a number */
                return "Illegal GID";
        }
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = groupname for setgid()", "setgid");
        break;
    }
#endif

#ifndef USE_WIN32
    /* setuid */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->uid=0;
        break;
    case CMD_SET_COPY:
        section->uid=new_service_options.uid;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "setuid"))
            break;
        pw=getpwnam(arg);
        if(pw) {
            section->uid=pw->pw_uid;
            return NULL; /* OK */
        }
        {
            char *tmp_str;
            section->uid=(uid_t)strtol(arg, &tmp_str, 10);
            if(tmp_str==arg || *tmp_str) /* not a number */
                return "Illegal UID";
        }
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = username for setuid()", "setuid");
        break;
    }
#endif

    /* sessionCacheSize */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->session_size=1000L;
        break;
    case CMD_SET_COPY:
        section->session_size=new_service_options.session_size;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "sessionCacheSize"))
            break;
        {
            char *tmp_str;
            section->session_size=strtol(arg, &tmp_str, 10);
            if(tmp_str==arg || *tmp_str) /* not a number */
                return "Illegal session cache size";
        }
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        s_log(LOG_NOTICE, "%-22s = %ld", "sessionCacheSize", 1000L);
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = session cache size", "sessionCacheSize");
        break;
    }

    /* sessionCacheTimeout */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->session_timeout=300L;
        break;
    case CMD_SET_COPY:
        section->session_timeout=new_service_options.session_timeout;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "sessionCacheTimeout") && strcasecmp(opt, "session"))
            break;
        {
            char *tmp_str;
            section->session_timeout=strtol(arg, &tmp_str, 10);
            if(tmp_str==arg || *tmp_str) /* not a number */
                return "Illegal session cache timeout";
        }
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        s_log(LOG_NOTICE, "%-22s = %ld seconds", "sessionCacheTimeout", 300L);
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = session cache timeout (in seconds)",
            "sessionCacheTimeout");
        break;
    }

    /* sessionResume */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.session_resume=1; /* enabled by default */
        break;
    case CMD_SET_COPY:
        section->option.session_resume=new_service_options.option.session_resume;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "sessionResume"))
            break;
        if(!strcasecmp(arg, "yes"))
            section->option.session_resume=1;
        else if(!strcasecmp(arg, "no"))
            section->option.session_resume=0;
        else
            return "The argument needs to be either 'yes' or 'no'";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = yes|no enable session resumption",
            "sessionResume");
        break;
    }

    /* sessiond */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.sessiond=0;
        memset(&section->sessiond_addr, 0, sizeof(SOCKADDR_UNION));
        section->sessiond_addr.in.sin_family=AF_INET;
        break;
    case CMD_SET_COPY:
        section->option.sessiond=new_service_options.option.sessiond;
        memcpy(&section->sessiond_addr, &new_service_options.sessiond_addr,
            sizeof(SOCKADDR_UNION));
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "sessiond"))
            break;
        section->option.sessiond=1;
#ifdef SSL_OP_NO_TICKET
        /* disable RFC4507 support introduced in OpenSSL 0.9.8f */
        /* this prevents session callbacks from being executed */
        section->ssl_options_set|=SSL_OP_NO_TICKET;
#endif
        if(!name2addr(&section->sessiond_addr, arg, 0))
            return "Failed to resolve sessiond server address";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = [host:]port use sessiond at host:port",
            "sessiond");
        break;
    }

#ifndef OPENSSL_NO_TLSEXT
    /* sni */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->servername_list_head=NULL;
        section->servername_list_tail=NULL;
        break;
    case CMD_SET_COPY:
        section->sni=
            str_dup_detached(new_service_options.sni);
        break;
    case CMD_FREE:
        str_free(section->sni);
        sni_free(section);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "sni"))
            break;
        str_free(section->sni);
        section->sni=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        {
            const char *tmp_str=sni_init(section);
            if(tmp_str)
                return tmp_str;
        }
        if(!section->option.client && section->sni)
            ++endpoints;
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = master_service:host_name for an SNI virtual service",
            "sni");
        break;
    }
#endif /* !defined(OPENSSL_NO_TLSEXT) */

    /* socket */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->sock_opts=socket_options_init();
        break;
    case CMD_SET_COPY:
        section->sock_opts=socket_options_dup(new_service_options.sock_opts);
        break;
    case CMD_FREE:
        socket_options_free(section->sock_opts);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "socket"))
            break;
        if(socket_option_parse(section->sock_opts, arg))
            return "Illegal socket option";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = a|l|r:option=value[:value]", "socket");
        s_log(LOG_NOTICE, "%25sset an option on accept/local/remote socket", "");
        break;
    }

#if OPENSSL_VERSION_NUMBER>=0x10100000L

    /* sslVersion */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        /* handled in sslVersionMax and sslVersionMin */
        break;
    case CMD_SET_COPY:
        /* handled in sslVersionMax and sslVersionMin */
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "sslVersion"))
            break;
        section->max_proto_version=
            section->min_proto_version=str_to_proto_version(arg);
        if(section->max_proto_version==-1)
            return "Invalid protocol version";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(section->max_proto_version && section->min_proto_version &&
                section->max_proto_version<section->min_proto_version)
            return "Invalid protocol version range";
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = all"
            "|SSLv3|TLSv1|TLSv1.1|TLSv1.2"
#ifdef TLS1_3_VERSION
            "|TLSv1.3"
#endif
            " TLS version", "sslVersion");
        break;
    }

    /* sslVersionMax */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->max_proto_version=0; /* highest supported */
        break;
    case CMD_SET_COPY:
        section->max_proto_version=new_service_options.max_proto_version;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "sslVersionMax"))
            break;
        section->max_proto_version=str_to_proto_version(arg);
        if(section->max_proto_version==-1)
            return "Invalid protocol version";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = all"
            "|SSLv3|TLSv1|TLSv1.1|TLSv1.2"
#ifdef TLS1_3_VERSION
            "|TLSv1.3"
#endif
            " TLS version", "sslVersionMax");
        break;
    }

    /* sslVersionMin */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->min_proto_version=TLS1_VERSION;
        break;
    case CMD_SET_COPY:
        section->min_proto_version=new_service_options.min_proto_version;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "sslVersionMin"))
            break;
        section->min_proto_version=str_to_proto_version(arg);
        if(section->min_proto_version==-1)
            return "Invalid protocol version";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = all"
            "|SSLv3|TLSv1|TLSv1.1|TLSv1.2"
#ifdef TLS1_3_VERSION
            "|TLSv1.3"
#endif
            " TLS version", "sslVersionMin");
        break;
    }

#else /* OPENSSL_VERSION_NUMBER<0x10100000L */

    /* sslVersion */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        tls_methods_set(section, NULL);
        break;
    case CMD_SET_COPY:
        section->client_method=new_service_options.client_method;
        section->server_method=new_service_options.server_method;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "sslVersion"))
            break;
        return tls_methods_set(section, arg);
    case CMD_INITIALIZE:
        {
            char *tmp_str=tls_methods_check(section);
            if(tmp_str)
                return tmp_str;
        }
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = all"
            "|SSLv2|SSLv3|TLSv1"
#if OPENSSL_VERSION_NUMBER>=0x10001000L
            "|TLSv1.1|TLSv1.2"
#endif /* OPENSSL_VERSION_NUMBER>=0x10001000L */
            " TLS method", "sslVersion");
        break;
    }

#endif /* OPENSSL_VERSION_NUMBER<0x10100000L */

#ifndef USE_FORK
    /* stack */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->stack_size=DEFAULT_STACK_SIZE;
        break;
    case CMD_SET_COPY:
        section->stack_size=new_service_options.stack_size;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "stack"))
            break;
        {
            char *tmp_str;
            section->stack_size=(size_t)strtol(arg, &tmp_str, 10);
            if(tmp_str==arg || *tmp_str) /* not a number */
                return "Illegal thread stack size";
        }
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        s_log(LOG_NOTICE, "%-22s = %d bytes", "stack", DEFAULT_STACK_SIZE);
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = thread stack size (in bytes)", "stack");
        break;
    }
#endif

#if OPENSSL_VERSION_NUMBER>=0x10000000L

    /* ticketKeySecret */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->ticket_key=NULL;
        break;
    case CMD_SET_COPY:
        section->ticket_key=key_dup(new_service_options.ticket_key);
        break;
    case CMD_FREE:
        key_free(section->ticket_key);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "ticketKeySecret"))
            break;
        section->ticket_key=key_read(arg, "ticketKeySecret");
        if(!section->ticket_key)
            return "Failed to read ticketKeySecret";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(!section->ticket_key)          /* ticketKeySecret not configured */
            break;
        if(section->option.client){
            s_log(LOG_NOTICE,
                    "ticketKeySecret is ignored in the client mode");
            break;
        }
        if(section->ticket_key && !section->ticket_mac)
            return "\"ticketKeySecret\" and \"ticketMacSecret\" must be set together";
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = secret key for encryption/decryption TLSv1.3 tickets",
            "ticketKeySecret");
        break;
    }

    /* ticketMacSecret */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->ticket_mac=NULL;
        break;
    case CMD_SET_COPY:
        section->ticket_mac=key_dup(new_service_options.ticket_mac);
        break;
    case CMD_FREE:
        key_free(section->ticket_mac);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "ticketMacSecret"))
            break;
        section->ticket_mac=key_read(arg, "ticketMacSecret");
        if(!section->ticket_mac)
            return "Failed to read ticketMacSecret";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(!section->ticket_mac)            /* ticketMacSecret not configured */
            break;
        if(section->option.client){
            s_log(LOG_NOTICE,
                    "ticketMacSecret is ignored in the client mode");
            break;
        }
        if(section->ticket_mac && !section->ticket_key)
            return "\"ticketKeySecret\" and \"ticketMacSecret\" must be set together";
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = key for HMAC operations on TLSv1.3 tickets",
            "ticketMacSecret");
        break;
    }

#endif /* OpenSSL 1.0.0 or later */

    /* TIMEOUTbusy */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->timeout_busy=300; /* 5 minutes */
        break;
    case CMD_SET_COPY:
        section->timeout_busy=new_service_options.timeout_busy;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "TIMEOUTbusy"))
            break;
        {
            char *tmp_str;
            section->timeout_busy=(int)strtol(arg, &tmp_str, 10);
            if(tmp_str==arg || *tmp_str) /* not a number */
                return "Illegal busy timeout";
        }
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        s_log(LOG_NOTICE, "%-22s = %d seconds", "TIMEOUTbusy", 300);
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = seconds to wait for expected data", "TIMEOUTbusy");
        break;
    }

    /* TIMEOUTclose */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->timeout_close=60; /* 1 minute */
        break;
    case CMD_SET_COPY:
        section->timeout_close=new_service_options.timeout_close;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "TIMEOUTclose"))
            break;
        {
            char *tmp_str;
            section->timeout_close=(int)strtol(arg, &tmp_str, 10);
            if(tmp_str==arg || *tmp_str) /* not a number */
                return "Illegal close timeout";
        }
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        s_log(LOG_NOTICE, "%-22s = %d seconds", "TIMEOUTclose", 60);
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = seconds to wait for close_notify",
            "TIMEOUTclose");
        break;
    }

    /* TIMEOUTconnect */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->timeout_connect=10; /* 10 seconds */
        break;
    case CMD_SET_COPY:
        section->timeout_connect=new_service_options.timeout_connect;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "TIMEOUTconnect"))
            break;
        {
            char *tmp_str;
            section->timeout_connect=(int)strtol(arg, &tmp_str, 10);
            if(tmp_str==arg || *tmp_str) /* not a number */
                return "Illegal connect timeout";
        }
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        s_log(LOG_NOTICE, "%-22s = %d seconds", "TIMEOUTconnect", 10);
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = seconds to connect remote host", "TIMEOUTconnect");
        break;
    }

    /* TIMEOUTidle */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->timeout_idle=43200; /* 12 hours */
        break;
    case CMD_SET_COPY:
        section->timeout_idle=new_service_options.timeout_idle;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "TIMEOUTidle"))
            break;
        {
            char *tmp_str;
            section->timeout_idle=(int)strtol(arg, &tmp_str, 10);
            if(tmp_str==arg || *tmp_str) /* not a number */
                return "Illegal idle timeout";
            return NULL; /* OK */
        }
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        s_log(LOG_NOTICE, "%-22s = %d seconds", "TIMEOUTidle", 43200);
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = seconds to keep an idle connection", "TIMEOUTidle");
        break;
    }

    /* transparent */
#ifndef USE_WIN32
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.transparent_src=0;
        section->option.transparent_dst=0;
        break;
    case CMD_SET_COPY:
        section->option.transparent_src=new_service_options.option.transparent_src;
        section->option.transparent_dst=new_service_options.option.transparent_dst;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "transparent"))
            break;
        if(!strcasecmp(arg, "none") || !strcasecmp(arg, "no")) {
            section->option.transparent_src=0;
            section->option.transparent_dst=0;
        } else if(!strcasecmp(arg, "source") || !strcasecmp(arg, "yes")) {
            section->option.transparent_src=1;
            section->option.transparent_dst=0;
        } else if(!strcasecmp(arg, "destination")) {
            section->option.transparent_src=0;
            section->option.transparent_dst=1;
        } else if(!strcasecmp(arg, "both")) {
            section->option.transparent_src=1;
            section->option.transparent_dst=1;
        } else
            return "Selected transparent proxy mode is not available";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(section->option.transparent_dst)
            ++endpoints;
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE,
            "%-22s = none|source|destination|both transparent proxy mode",
            "transparent");
        break;
    }
#endif

    /* verify */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.request_cert=0;
        break;
    case CMD_SET_COPY:
        section->option.request_cert=new_service_options.option.request_cert;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "verify"))
            break;
        {
            char *tmp_str;
            int tmp_int=(int)strtol(arg, &tmp_str, 10);
            if(tmp_str==arg || *tmp_str || tmp_int<0 || tmp_int>4)
                return "Bad verify level";
            section->option.request_cert=1;
            section->option.require_cert=(tmp_int>=2);
            section->option.verify_chain=(tmp_int>=1 && tmp_int<=3);
            section->option.verify_peer=(tmp_int>=3);
        }
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if((section->option.verify_chain || section->option.verify_peer) &&
                !section->ca_file && !section->ca_dir)
            return "Either \"CAfile\" or \"CApath\" has to be configured";
        break;
    case CMD_PRINT_DEFAULTS:
        s_log(LOG_NOTICE, "%-22s = none", "verify");
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE,
            "%-22s = level of peer certificate verification", "verify");
        break;
    }

    /* verifyChain */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.verify_chain=0;
        break;
    case CMD_SET_COPY:
        section->option.verify_chain=new_service_options.option.verify_chain;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "verifyChain"))
            break;
        if(!strcasecmp(arg, "yes")) {
            section->option.request_cert=1;
            section->option.require_cert=1;
            section->option.verify_chain=1;
        } else if(!strcasecmp(arg, "no")) {
            section->option.verify_chain=0;
        } else {
            return "The argument needs to be either 'yes' or 'no'";
        }
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = yes|no verify certificate chain",
            "verifyChain");
        break;
    }

    /* verifyPeer */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.verify_peer=0;
        break;
    case CMD_SET_COPY:
        section->option.verify_peer=new_service_options.option.verify_peer;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "verifyPeer"))
            break;
        if(!strcasecmp(arg, "yes")) {
            section->option.request_cert=1;
            section->option.require_cert=1;
            section->option.verify_peer=1;
        } else if(!strcasecmp(arg, "no")) {
            section->option.verify_peer=0;
        } else {
            return "The argument needs to be either 'yes' or 'no'";
        }
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = yes|no verify peer certificate",
            "verifyPeer");
        break;
    }

    /* final checks */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        break;
    case CMD_SET_COPY:
        break;
    case CMD_FREE:
        str_free(section->chain);
        if(section->session)
            SSL_SESSION_free(section->session);
        if(section->ctx)
            SSL_CTX_free(section->ctx);
        str_free(section->servname);
        if(section==&service_options || section==&new_service_options)
            memset(section, 0, sizeof(SERVICE_OPTIONS));
        else
            str_free(section);
        break;
    case CMD_SET_VALUE:
        return option_not_found;
    case CMD_INITIALIZE:
        if(section!=&new_service_options) { /* daemon mode checks */
            if(endpoints!=2)
                return "Each service must define two endpoints";
        } else { /* inetd mode checks */
            if(section->option.accept)
                return "'accept' option is only allowed in a [section]";
            /* no need to check for section->sni in inetd mode,
               as it requires valid sections to be set */
            if(endpoints!=1)
                return "Inetd mode must define one endpoint";
        }
#ifdef SSL_OP_NO_TICKET
        /* disable RFC4507 support introduced in OpenSSL 0.9.8f */
        /* OpenSSL 1.1.1 is required to serialize application data
         * into session tickets */
        /* server mode sections need it for the "redirect" option
         * and connect address session persistence */
        if(OpenSSL_version_num()<0x10101000L &&
                !section->option.client &&
                !section->option.connect_before_ssl)
            section->ssl_options_set|=SSL_OP_NO_TICKET;
#endif /* SSL_OP_NO_TICKET */
        if(context_init(section)) /* initialize TLS context */
            return "Failed to initialize TLS context";
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        break;
    }

    return NULL; /* OK */
}

/**************************************** validate and initialize configuration */

#ifndef OPENSSL_NO_TLSEXT

NOEXPORT const char *sni_init(SERVICE_OPTIONS *section) {
    char *tmp_str;
    SERVICE_OPTIONS *tmpsrv;

    /* server mode: update servername_list based on the SNI option */
    if(!section->option.client && section->sni) {
        tmp_str=strchr(section->sni, ':');
        if(!tmp_str)
            return "Invalid SNI parameter format";
        *tmp_str++='\0';
        for(tmpsrv=new_service_options.next; tmpsrv; tmpsrv=tmpsrv->next)
            if(!strcmp(tmpsrv->servname, section->sni))
                break;
        if(!tmpsrv)
            return "SNI section name not found";
        if(tmpsrv->option.client)
            return "SNI master service is a TLS client";
        if(tmpsrv->servername_list_tail) {
            tmpsrv->servername_list_tail->next=str_alloc_detached(sizeof(SERVERNAME_LIST));
            tmpsrv->servername_list_tail=tmpsrv->servername_list_tail->next;
        } else { /* first virtual service */
            tmpsrv->servername_list_head=
                tmpsrv->servername_list_tail=
                str_alloc_detached(sizeof(SERVERNAME_LIST));
            tmpsrv->ssl_options_set|=
                SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION;
        }
        /* a slave section reference is needed to prevent a race condition
           while switching to a section after configuration file reload */
        service_up_ref(section);
        tmpsrv->servername_list_tail->servername=str_dup_detached(tmp_str);
        tmpsrv->servername_list_tail->opt=section;
        tmpsrv->servername_list_tail->next=NULL;
        /* always negotiate a new session on renegotiation, as the TLS
         * context settings (including access control) may be different */
        section->ssl_options_set|=
            SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION;
    }

    /* client mode: setup SNI default based on 'protocolHost' and 'connect' options */
    if(section->option.client && !section->sni) {
        /* setup host_name for SNI, prefer SNI and protocolHost if specified */
        if(section->protocol_host) /* 'protocolHost' option */
            section->sni=str_dup_detached(section->protocol_host);
        else if(section->connect_addr.names) /* 'connect' option */
            section->sni=str_dup_detached(section->connect_addr.names->name); /* first hostname */
        if(section->sni) { /* either 'protocolHost' or 'connect' specified */
            tmp_str=strrchr(section->sni, ':');
            if(tmp_str) { /* 'host:port' -> drop ':port' */
                *tmp_str='\0';
            } else { /* 'port' -> default to 'localhost' */
                str_free(section->sni);
                section->sni=str_dup_detached("localhost");
            }
        }
    }
    return NULL;
}

NOEXPORT void sni_free(SERVICE_OPTIONS *section) {
    SERVERNAME_LIST *curr=section->servername_list_head;
    while(curr) {
        SERVERNAME_LIST *next=curr->next;
        str_free(curr->servername);
        service_free(curr->opt); /* free the slave section */
        str_free(curr);
        curr=next;
    }
    section->servername_list_head=NULL;
    section->servername_list_tail=NULL;
}

#endif /* !defined(OPENSSL_NO_TLSEXT) */

/**************************************** modern TLS version handling */

#if OPENSSL_VERSION_NUMBER>=0x10100000L

NOEXPORT int str_to_proto_version(const char *name) {
    if(!strcasecmp(name, "all"))
        return 0;
    if(!strcasecmp(name, "SSLv3"))
        return SSL3_VERSION;
    if(!strcasecmp(name, "TLSv1"))
        return TLS1_VERSION;
    if(!strcasecmp(name, "TLSv1.1"))
        return TLS1_1_VERSION;
    if(!strcasecmp(name, "TLSv1.2"))
        return TLS1_2_VERSION;
#ifdef TLS1_3_VERSION
    if(!strcasecmp(name, "TLSv1.3"))
        return TLS1_3_VERSION;
#endif
    return -1;
}

/**************************************** deprecated TLS version handling */

#else /* OPENSSL_VERSION_NUMBER<0x10100000L */

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif /* __GNUC__ */

NOEXPORT char *tls_methods_set(SERVICE_OPTIONS *section, const char *arg) {
    if(!arg) { /* defaults */
        section->client_method=(SSL_METHOD *)SSLv23_client_method();
        section->server_method=(SSL_METHOD *)SSLv23_server_method();
    } else if(!strcasecmp(arg, "all")) {
        section->client_method=(SSL_METHOD *)SSLv23_client_method();
        section->server_method=(SSL_METHOD *)SSLv23_server_method();
    } else if(!strcasecmp(arg, "SSLv2")) {
#ifndef OPENSSL_NO_SSL2
        section->client_method=(SSL_METHOD *)SSLv2_client_method();
        section->server_method=(SSL_METHOD *)SSLv2_server_method();
#else /* OPENSSL_NO_SSL2 */
        return "SSLv2 not supported";
#endif /* !OPENSSL_NO_SSL2 */
    } else if(!strcasecmp(arg, "SSLv3")) {
#ifndef OPENSSL_NO_SSL3
        section->client_method=(SSL_METHOD *)SSLv3_client_method();
        section->server_method=(SSL_METHOD *)SSLv3_server_method();
#else /* OPENSSL_NO_SSL3 */
        return "SSLv3 not supported";
#endif /* !OPENSSL_NO_SSL3 */
    } else if(!strcasecmp(arg, "TLSv1")) {
#ifndef OPENSSL_NO_TLS1
        section->client_method=(SSL_METHOD *)TLSv1_client_method();
        section->server_method=(SSL_METHOD *)TLSv1_server_method();
#else /* OPENSSL_NO_TLS1 */
        return "TLSv1 not supported";
#endif /* !OPENSSL_NO_TLS1 */
    } else if(!strcasecmp(arg, "TLSv1.1")) {
#ifndef OPENSSL_NO_TLS1_1
        section->client_method=(SSL_METHOD *)TLSv1_1_client_method();
        section->server_method=(SSL_METHOD *)TLSv1_1_server_method();
#else /* OPENSSL_NO_TLS1_1 */
        return "TLSv1.1 not supported";
#endif /* !OPENSSL_NO_TLS1_1 */
    } else if(!strcasecmp(arg, "TLSv1.2")) {
#ifndef OPENSSL_NO_TLS1_2
        section->client_method=(SSL_METHOD *)TLSv1_2_client_method();
        section->server_method=(SSL_METHOD *)TLSv1_2_server_method();
#else /* OPENSSL_NO_TLS1_2 */
        return "TLSv1.2 not supported";
#endif /* !OPENSSL_NO_TLS1_2 */
    } else
        return "Incorrect version of TLS protocol";
    return NULL; /* OK */
}

NOEXPORT char *tls_methods_check(SERVICE_OPTIONS *section) {
#ifdef USE_FIPS
    if(new_global_options.option.fips) {
#ifndef OPENSSL_NO_SSL2
        if(section->option.client ?
                section->client_method==(SSL_METHOD *)SSLv2_client_method() :
                section->server_method==(SSL_METHOD *)SSLv2_server_method())
            return "\"sslVersion = SSLv2\" not supported in FIPS mode";
#endif /* !OPENSSL_NO_SSL2 */
#ifndef OPENSSL_NO_SSL3
        if(section->option.client ?
                section->client_method==(SSL_METHOD *)SSLv3_client_method() :
                section->server_method==(SSL_METHOD *)SSLv3_server_method())
            return "\"sslVersion = SSLv3\" not supported in FIPS mode";
#endif /* !OPENSSL_NO_SSL3 */
    }
#else /* USE_FIPS */
    (void)section; /* squash the unused parameter warning */
#endif /* USE_FIPS */
    return NULL;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif /* __GNUC__ */

#endif /* OPENSSL_VERSION_NUMBER<0x10100000L */

/**************************************** facility/debug level */

typedef struct {
    const char *name;
    int value;
} facilitylevel;

NOEXPORT const char *parse_debug_level(char *arg, SERVICE_OPTIONS *section) {
    facilitylevel *fl;

/* facilities only make sense on unix */
#if !defined (USE_WIN32) && !defined (__vms)
    facilitylevel facilities[] = {
        {"auth", LOG_AUTH},     {"cron", LOG_CRON},     {"daemon", LOG_DAEMON},
        {"kern", LOG_KERN},     {"lpr", LOG_LPR},       {"mail", LOG_MAIL},
        {"news", LOG_NEWS},     {"syslog", LOG_SYSLOG}, {"user", LOG_USER},
        {"uucp", LOG_UUCP},     {"local0", LOG_LOCAL0}, {"local1", LOG_LOCAL1},
        {"local2", LOG_LOCAL2}, {"local3", LOG_LOCAL3}, {"local4", LOG_LOCAL4},
        {"local5", LOG_LOCAL5}, {"local6", LOG_LOCAL6}, {"local7", LOG_LOCAL7},

        /* some facilities are not defined on all Unices */
#ifdef LOG_AUTHPRIV
        {"authpriv", LOG_AUTHPRIV},
#endif
#ifdef LOG_FTP
        {"ftp", LOG_FTP},
#endif
#ifdef LOG_NTP
        {"ntp", LOG_NTP},
#endif
        {NULL, 0}
    };
#endif /* USE_WIN32, __vms */

    facilitylevel levels[] = {
        {"emerg", LOG_EMERG},     {"alert", LOG_ALERT},
        {"crit", LOG_CRIT},       {"err", LOG_ERR},
        {"warning", LOG_WARNING}, {"notice", LOG_NOTICE},
        {"info", LOG_INFO},       {"debug", LOG_DEBUG},
        {NULL, -1}
    };

/* facilities only make sense on Unix */
#if !defined (USE_WIN32) && !defined (__vms)
    if(section==&new_service_options && strchr(arg, '.')) {
        /* a facility was specified in the global options */
        new_global_options.log_facility=-1;
        arg=strtok(arg, "."); /* break it up */

        for(fl=facilities; fl->name; ++fl) {
            if(!strcasecmp(fl->name, arg)) {
                new_global_options.log_facility=fl->value;
                break;
            }
        }
        if(new_global_options.log_facility==-1)
            return "Illegal syslog facility";
        arg=strtok(NULL, ".");    /* set to the remainder */
    }
#endif /* USE_WIN32, __vms */

    /* time to check the syslog level */
    if(arg && strlen(arg)==1 && *arg>='0' && *arg<='7') {
        section->log_level=*arg-'0';
        return NULL; /* OK */
    }
    section->log_level=8;    /* illegal level */
    for(fl=levels; fl->name; ++fl) {
        if(!strcasecmp(fl->name, arg)) {
            section->log_level=fl->value;
            break;
        }
    }
    if(section->log_level==8)
        return "Illegal debug level"; /* FAILED */
    return NULL; /* OK */
}

/**************************************** TLS options */

NOEXPORT uint64_t parse_ssl_option(char *arg) {
    const SSL_OPTION *option;

    for(option=ssl_opts; option->name; ++option)
        if(!strcasecmp(option->name, arg))
            return option->value;
    return INVALID_SSL_OPTION; /* FAILED */
}

NOEXPORT void print_ssl_options(void) {
    const SSL_OPTION *option;

    s_log(LOG_NOTICE, " ");
    s_log(LOG_NOTICE, "Supported TLS options:");
    for(option=ssl_opts; option->name; ++option)
        s_log(LOG_NOTICE, "options = %s", option->name);
}

/**************************************** read PSK file */

#ifndef OPENSSL_NO_PSK

NOEXPORT PSK_KEYS *psk_read(char *key_file) {
    DISK_FILE *df;
    char line[CONFLINELEN], *key_str;
    unsigned char *key_buf;
    long key_len;
    PSK_KEYS *head=NULL, *tail=NULL, *curr;
    int line_number=0;

    if(file_permissions(key_file))
        return NULL;
    df=file_open(key_file, FILE_MODE_READ);
    if(!df) {
        s_log(LOG_ERR, "Cannot open PSKsecrets file");
        return NULL;
    }
    while(file_getline(df, line, CONFLINELEN)>=0) {
        ++line_number;
        if(!line[0]) /* empty line */
            continue;
        key_str=strchr(line, ':');
        if(!key_str) {
            s_log(LOG_ERR,
                "PSKsecrets line %d: Not in identity:key format",
                line_number);
            file_close(df);
            psk_free(head);
            return NULL;
        }
        *key_str++='\0';
        if(strlen(line)+1>PSK_MAX_IDENTITY_LEN) { /* with the trailing '\0' */
            s_log(LOG_ERR,
                "PSKsecrets line %d: Identity longer than %d characters",
                line_number, PSK_MAX_IDENTITY_LEN);
            file_close(df);
            psk_free(head);
            return NULL;
        }
        key_buf=OPENSSL_hexstr2buf(key_str, &key_len);
        if(key_buf) { /* a valid hexadecimal value */
            s_log(LOG_INFO, "PSKsecrets line %d: "
                "%ld-byte hexadecimal key configured for identity \"%s\"",
                line_number, key_len, line);
        } else { /* not a valid hexadecimal value -> copy as a string */
            key_len=(long)strlen(key_str);
            key_buf=OPENSSL_malloc((size_t)key_len);
            memcpy(key_buf, key_str, (size_t)key_len);
            s_log(LOG_INFO, "PSKsecrets line %d: "
                "%ld-byte ASCII key configured for identity \"%s\"",
                line_number, key_len, line);
        }
        if(key_len>PSK_MAX_PSK_LEN) {
            s_log(LOG_ERR,
                "PSKsecrets line %d: Key longer than %d bytes",
                line_number, PSK_MAX_PSK_LEN);
            OPENSSL_free(key_buf);
            file_close(df);
            psk_free(head);
            return NULL;
        }
        if(key_len<16) {
            /* shorter keys are unlikely to have sufficient entropy */
            s_log(LOG_ERR,
                "PSKsecrets line %d: Key shorter than 16 bytes",
                line_number);
            OPENSSL_free(key_buf);
            file_close(df);
            psk_free(head);
            return NULL;
        }
        curr=str_alloc_detached(sizeof(PSK_KEYS));
        curr->identity=str_dup_detached(line);
        curr->key_val=str_alloc_detached((size_t)key_len);
        memcpy(curr->key_val, key_buf, (size_t)key_len);
        OPENSSL_free(key_buf);
        curr->key_len=(unsigned)key_len;
        curr->next=NULL;
        if(head)
            tail->next=curr;
        else
            head=curr;
        tail=curr;
    }
    file_close(df);
    return head;
}

NOEXPORT PSK_KEYS *psk_dup(PSK_KEYS *src) {
    PSK_KEYS *head=NULL, *tail=NULL, *curr;

    for(; src; src=src->next) {
        curr=str_alloc_detached(sizeof(PSK_KEYS));
        curr->identity=str_dup_detached(src->identity);
        curr->key_val=str_alloc_detached(src->key_len);
        memcpy(curr->key_val, src->key_val, src->key_len);
        curr->key_len=src->key_len;
        curr->next=NULL;
        if(head)
            tail->next=curr;
        else
            head=curr;
        tail=curr;
    }
    return head;
}

NOEXPORT void psk_free(PSK_KEYS *head) {
    while(head) {
        PSK_KEYS *next=head->next;
        str_free_const(head->identity);
        str_free(head->key_val);
        str_free(head);
        head=next;
    }
}

#endif

/**************************************** read ticket key */

#if OPENSSL_VERSION_NUMBER>=0x10000000L

NOEXPORT TICKET_KEY *key_read(char *arg, const char *option) {
    char *key_str;
    unsigned char *key_buf;
    long key_len;
    TICKET_KEY *head=NULL;

    key_str=str_dup_detached(arg);
    key_buf=OPENSSL_hexstr2buf(key_str, &key_len);
    if(key_buf)
        if((key_len == 16) || (key_len == 32)) /* a valid 16 or 32 byte hexadecimal value */
            s_log(LOG_INFO, "%s configured", option);
        else { /* not a valid length */
            s_log(LOG_ERR, "%s value has %ld bytes instead of required 16 or 32 bytes",
                option, key_len);
            OPENSSL_free(key_buf);
            key_free(head);
            return NULL;
        }
    else { /* not a valid hexadecimal form */
        s_log(LOG_ERR, "Required %s is 16 or 32 byte hexadecimal key", option);
        key_free(head);
        return NULL;
    }
    head=str_alloc_detached(sizeof(TICKET_KEY));
    head->key_val=str_alloc_detached((size_t)key_len);
    memcpy(head->key_val, key_buf, (size_t)key_len);
    OPENSSL_free(key_buf);
    head->key_len=(int)key_len;
    return head;
}

NOEXPORT TICKET_KEY *key_dup(TICKET_KEY *src) {
    TICKET_KEY *head=NULL;

    if (src) {
        head=str_alloc_detached(sizeof(TICKET_KEY));
        head->key_val=(unsigned char *)str_dup_detached((char *)src->key_val);
        head->key_len=src->key_len;
    }
    return head;
}

NOEXPORT void key_free(TICKET_KEY *head) {
    if (head) {
        str_free(head->key_val);
        str_free(head);
    }
}

#endif /* OpenSSL 1.0.0 or later */

/**************************************** socket options */

#define VAL_TAB {NULL, NULL, NULL}

SOCK_OPT sock_opts_def[]={
    {"SO_DEBUG",        SOL_SOCKET,     SO_DEBUG,        TYPE_FLAG,     VAL_TAB},
    {"SO_DONTROUTE",    SOL_SOCKET,     SO_DONTROUTE,    TYPE_FLAG,     VAL_TAB},
    {"SO_KEEPALIVE",    SOL_SOCKET,     SO_KEEPALIVE,    TYPE_FLAG,     VAL_TAB},
    {"SO_LINGER",       SOL_SOCKET,     SO_LINGER,       TYPE_LINGER,   VAL_TAB},
    {"SO_OOBINLINE",    SOL_SOCKET,     SO_OOBINLINE,    TYPE_FLAG,     VAL_TAB},
    {"SO_RCVBUF",       SOL_SOCKET,     SO_RCVBUF,       TYPE_INT,      VAL_TAB},
    {"SO_SNDBUF",       SOL_SOCKET,     SO_SNDBUF,       TYPE_INT,      VAL_TAB},
#ifdef SO_RCVLOWAT
    {"SO_RCVLOWAT",     SOL_SOCKET,     SO_RCVLOWAT,     TYPE_INT,      VAL_TAB},
#endif
#ifdef SO_SNDLOWAT
    {"SO_SNDLOWAT",     SOL_SOCKET,     SO_SNDLOWAT,     TYPE_INT,      VAL_TAB},
#endif
#ifdef SO_RCVTIMEO
    {"SO_RCVTIMEO",     SOL_SOCKET,     SO_RCVTIMEO,     TYPE_TIMEVAL,  VAL_TAB},
#endif
#ifdef SO_SNDTIMEO
    {"SO_SNDTIMEO",     SOL_SOCKET,     SO_SNDTIMEO,     TYPE_TIMEVAL,  VAL_TAB},
#endif
#ifdef USE_WIN32
    {"SO_EXCLUSIVEADDRUSE", SOL_SOCKET, SO_EXCLUSIVEADDRUSE, TYPE_FLAG, VAL_TAB},
#endif
    {"SO_REUSEADDR",    SOL_SOCKET,     SO_REUSEADDR,    TYPE_FLAG,     VAL_TAB},
#ifdef SO_BINDTODEVICE
    {"SO_BINDTODEVICE", SOL_SOCKET,     SO_BINDTODEVICE, TYPE_STRING,   VAL_TAB},
#endif
#ifdef SOL_TCP
#ifdef TCP_KEEPCNT
    {"TCP_KEEPCNT",     SOL_TCP,        TCP_KEEPCNT,     TYPE_INT,      VAL_TAB},
#endif
#ifdef TCP_KEEPIDLE
    {"TCP_KEEPIDLE",    SOL_TCP,        TCP_KEEPIDLE,    TYPE_INT,      VAL_TAB},
#endif
#ifdef TCP_KEEPINTVL
    {"TCP_KEEPINTVL",   SOL_TCP,        TCP_KEEPINTVL,   TYPE_INT,      VAL_TAB},
#endif
#endif /* SOL_TCP */
#ifdef IP_TOS
    {"IP_TOS",          IPPROTO_IP,     IP_TOS,          TYPE_INT,      VAL_TAB},
#endif
#ifdef IP_TTL
    {"IP_TTL",          IPPROTO_IP,     IP_TTL,          TYPE_INT,      VAL_TAB},
#endif
#ifdef IP_MAXSEG
    {"TCP_MAXSEG",      IPPROTO_TCP,    TCP_MAXSEG,      TYPE_INT,      VAL_TAB},
#endif
    {"TCP_NODELAY",     IPPROTO_TCP,    TCP_NODELAY,     TYPE_FLAG,     VAL_TAB},
#ifdef IP_FREEBIND
    {"IP_FREEBIND",     IPPROTO_IP,     IP_FREEBIND,     TYPE_FLAG,     VAL_TAB},
#endif
#ifdef IP_BINDANY
    {"IP_BINDANY",      IPPROTO_IP,     IP_BINDANY,      TYPE_FLAG,     VAL_TAB},
#endif
#ifdef IPV6_BINDANY
    {"IPV6_BINDANY",    IPPROTO_IPV6,   IPV6_BINDANY,    TYPE_FLAG,     VAL_TAB},
#endif
#ifdef IPV6_V6ONLY
    {"IPV6_V6ONLY",     IPPROTO_IPV6,   IPV6_V6ONLY,     TYPE_FLAG,     VAL_TAB},
#endif
    {NULL,              0,              0,               TYPE_NONE,     VAL_TAB}
};

NOEXPORT SOCK_OPT *socket_options_init(void) {
#ifdef USE_WIN32
    DWORD version;
    int major, minor;
#endif

    SOCK_OPT *opt=str_alloc_detached(sizeof sock_opts_def);
    memcpy(opt, sock_opts_def, sizeof sock_opts_def);

#ifdef USE_WIN32
#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif
    version=GetVersion();
    major=LOBYTE(LOWORD(version));
    minor=HIBYTE(LOWORD(version));
    s_log(LOG_DEBUG, "Running on Windows %d.%d", major, minor);

    if(major>5) /* Vista or later */
        socket_option_set_int(opt, "SO_EXCLUSIVEADDRUSE", 0, 1); /* accepting socket */
#else
    socket_option_set_int(opt, "SO_REUSEADDR", 0, 1); /* accepting socket */
#endif
    socket_option_set_int(opt, "TCP_NODELAY", 1, 1); /* local socket */
    socket_option_set_int(opt, "TCP_NODELAY", 2, 1); /* remote socket */
    return opt;
}

NOEXPORT void socket_option_set_int(SOCK_OPT *opt, const char *name, int type, int value) {
    for(; opt->opt_str; ++opt) {
        if(!strcmp(name, opt->opt_str)) {
            opt->opt_val[type]=str_alloc_detached(sizeof(OPT_UNION));
            opt->opt_val[type]->i_val=value;
        }
    }
}

NOEXPORT SOCK_OPT *socket_options_dup(SOCK_OPT *src) {
    SOCK_OPT *dst=str_alloc_detached(sizeof sock_opts_def);
    SOCK_OPT *ptr;

    memcpy(dst, sock_opts_def, sizeof sock_opts_def);
    for(ptr=dst; src->opt_str; ++src, ++ptr) {
        int type;
        for(type=0; type<3; ++type) {
            if(src->opt_val[type]) {
                ptr->opt_val[type]=str_alloc_detached(sizeof(OPT_UNION));
                memcpy(ptr->opt_val[type],
                    src->opt_val[type], sizeof(OPT_UNION));
            }
        }
    }
    return dst;
}

NOEXPORT void socket_options_free(SOCK_OPT *opt) {
    SOCK_OPT *ptr;
    if(!opt) {
        s_log(LOG_ERR, "INTERNAL ERROR: Socket options not initialized");
        return;
    }
    for(ptr=opt; ptr->opt_str; ++ptr) {
        int type;
        for(type=0; type<3; ++type)
            str_free(ptr->opt_val[type]);
    }
    str_free(opt);
}

NOEXPORT int socket_options_print(void) {
    SOCK_OPT *opt, *ptr;

    s_log(LOG_NOTICE, " ");
    s_log(LOG_NOTICE, "Socket option defaults:");
    s_log(LOG_NOTICE,
        "    Option Name         |  Accept  |   Local  |  Remote  |OS default");
    s_log(LOG_NOTICE,
        "    --------------------+----------+----------+----------+----------");

    opt=socket_options_init();
    for(ptr=opt; ptr->opt_str; ++ptr) {
        SOCKET fd;
        socklen_t optlen;
        OPT_UNION val;
        char *ta, *tl, *tr, *td;

        /* get OS default value */
#if defined(AF_INET6) && defined(IPPROTO_IPV6)
        if(ptr->opt_level==IPPROTO_IPV6)
            fd=socket(AF_INET6, SOCK_STREAM, 0);
        else
#endif
            fd=socket(AF_INET, SOCK_STREAM, 0);
        optlen=sizeof val;
        if(getsockopt(fd, ptr->opt_level,
                ptr->opt_name, (void *)&val, &optlen)) {
            switch(get_last_socket_error()) {
            case S_ENOPROTOOPT:
            case S_EOPNOTSUPP:
                td=str_dup("write-only");
                break;
            default:
                s_log(LOG_ERR, "Failed to get %s OS default", ptr->opt_str);
                sockerror("getsockopt");
                closesocket(fd);
                return 1; /* FAILED */
            }
        } else
            td=socket_option_text(ptr->opt_type, &val);
        closesocket(fd);

        /* get stunnel default values */
        ta=socket_option_text(ptr->opt_type, ptr->opt_val[0]);
        tl=socket_option_text(ptr->opt_type, ptr->opt_val[1]);
        tr=socket_option_text(ptr->opt_type, ptr->opt_val[2]);

        /* print collected data and free the allocated memory */
        s_log(LOG_NOTICE, "    %-20s|%10s|%10s|%10s|%10s",
            ptr->opt_str, ta, tl, tr, td);
        str_free(ta); str_free(tl); str_free(tr); str_free(td);
    }
    socket_options_free(opt);
    return 0; /* OK */
}

NOEXPORT char *socket_option_text(VAL_TYPE type, OPT_UNION *val) {
    if(!val)
        return str_dup("    --    ");
    switch(type) {
    case TYPE_FLAG:
        return str_printf("%s", val->i_val ? "yes" : "no");
    case TYPE_INT:
        return str_printf("%d", val->i_val);
    case TYPE_LINGER:
        return str_printf("%d:%d",
            val->linger_val.l_onoff, val->linger_val.l_linger);
    case TYPE_TIMEVAL:
        return str_printf("%d:%d",
            (int)val->timeval_val.tv_sec, (int)val->timeval_val.tv_usec);
    case TYPE_STRING:
        return str_printf("%s", val->c_val);
    case TYPE_NONE:
        return str_dup("   none   "); /* internal error? */
    }
    return str_dup("  Ooops?  "); /* internal error? */
}

NOEXPORT int socket_option_parse(SOCK_OPT *opt, char *arg) {
    int socket_type; /* 0-accept, 1-local, 2-remote */
    char *opt_val_str, *opt_val2_str, *tmp_str;
    OPT_UNION opt_val;

    if(arg[1]!=':')
        return 1; /* FAILED */
    switch(arg[0]) {
    case 'a':
        socket_type=0; break;
    case 'l':
        socket_type=1; break;
    case 'r':
        socket_type=2; break;
    default:
        return 1; /* FAILED */
    }
    arg+=2;
    opt_val_str=strchr(arg, '=');
    if(!opt_val_str) /* no '='? */
        return 1; /* FAILED */
    *opt_val_str++='\0';

    for(; opt->opt_str && strcmp(arg, opt->opt_str); ++opt)
        ;
    if(!opt->opt_str)
        return 1; /* FAILED */

    switch(opt->opt_type) {
    case TYPE_FLAG:
        if(!strcasecmp(opt_val_str, "yes") || !strcmp(opt_val_str, "1")) {
            opt_val.i_val=1;
            break; /* OK */
        }
        if(!strcasecmp(opt_val_str, "no") || !strcmp(opt_val_str, "0")) {
            opt_val.i_val=0;
            break; /* OK */
        }
        return 1; /* FAILED */
    case TYPE_INT:
        opt_val.i_val=(int)strtol(opt_val_str, &tmp_str, 10);
        if(tmp_str==arg || *tmp_str) /* not a number */
            return 1; /* FAILED */
        break; /* OK */
    case TYPE_LINGER:
        opt_val2_str=strchr(opt_val_str, ':');
        if(opt_val2_str) {
            *opt_val2_str++='\0';
            opt_val.linger_val.l_linger=
                (u_short)strtol(opt_val2_str, &tmp_str, 10);
            if(tmp_str==arg || *tmp_str) /* not a number */
                return 1; /* FAILED */
        } else {
            opt_val.linger_val.l_linger=0;
        }
        opt_val.linger_val.l_onoff=
            (u_short)strtol(opt_val_str, &tmp_str, 10);
        if(tmp_str==arg || *tmp_str) /* not a number */
            return 1; /* FAILED */
        break; /* OK */
    case TYPE_TIMEVAL:
        opt_val2_str=strchr(opt_val_str, ':');
        if(opt_val2_str) {
            *opt_val2_str++='\0';
            opt_val.timeval_val.tv_usec=
                (int)strtol(opt_val2_str, &tmp_str, 10);
            if(tmp_str==arg || *tmp_str) /* not a number */
                return 1; /* FAILED */
        } else {
            opt_val.timeval_val.tv_usec=0;
        }
        opt_val.timeval_val.tv_sec=
            (int)strtol(opt_val_str, &tmp_str, 10);
        if(tmp_str==arg || *tmp_str) /* not a number */
            return 1; /* FAILED */
        break; /* OK */
    case TYPE_STRING:
        if(strlen(opt_val_str)+1>sizeof(OPT_UNION))
            return 1; /* FAILED */
        strcpy(opt_val.c_val, opt_val_str);
        break; /* OK */
    default:
        return 1; /* FAILED */
    }
    str_free(opt->opt_val[socket_type]);
    opt->opt_val[socket_type]=str_alloc_detached(sizeof(OPT_UNION));
    memcpy(opt->opt_val[socket_type], &opt_val, sizeof(OPT_UNION));
    return 0;
}

/**************************************** OCSP */

#ifndef OPENSSL_NO_OCSP

NOEXPORT unsigned long parse_ocsp_flag(char *arg) {
    struct {
        const char *name;
        unsigned long value;
    } ocsp_opts[] = {
        {"NOCERTS", OCSP_NOCERTS},
        {"NOINTERN", OCSP_NOINTERN},
        {"NOSIGS", OCSP_NOSIGS},
        {"NOCHAIN", OCSP_NOCHAIN},
        {"NOVERIFY", OCSP_NOVERIFY},
        {"NOEXPLICIT", OCSP_NOEXPLICIT},
        {"NOCASIGN", OCSP_NOCASIGN},
        {"NODELEGATED", OCSP_NODELEGATED},
        {"NOCHECKS", OCSP_NOCHECKS},
        {"TRUSTOTHER", OCSP_TRUSTOTHER},
        {"RESPID_KEY", OCSP_RESPID_KEY},
        {"NOTIME", OCSP_NOTIME},
        {NULL, 0}
    }, *option;

    for(option=ocsp_opts; option->name; ++option)
        if(!strcasecmp(option->name, arg))
            return option->value;
    return 0; /* FAILED */
}

#endif /* !defined(OPENSSL_NO_OCSP) */

/**************************************** engine */

#ifndef OPENSSL_NO_ENGINE

#define MAX_ENGINES 256
static ENGINE *engines[MAX_ENGINES]; /* table of engines for config parser */
static int current_engine;
static int engine_initialized;

NOEXPORT void engine_reset_list(void) {
    current_engine=-1;
    engine_initialized=1;
}

NOEXPORT const char *engine_auto(void) {
    ENGINE *e;

    s_log(LOG_INFO, "Enabling automatic engine support");
    ENGINE_register_all_complete();
    /* rebuild the internal list of engines */
    engine_reset_list();
    for(e=ENGINE_get_first(); e; e=ENGINE_get_next(e)) {
        if(++current_engine>=MAX_ENGINES)
            return "Too many open engines";
        engines[current_engine]=e;
        s_log(LOG_INFO, "Engine #%d (%s) registered",
            current_engine+1, ENGINE_get_id(e));
    }
    s_log(LOG_INFO, "Automatic engine support enabled");
    return NULL; /* OK */
}

NOEXPORT const char *engine_open(const char *name) {
    engine_init(); /* initialize the previous engine (if any) */
    if(++current_engine>=MAX_ENGINES)
        return "Too many open engines";
    s_log(LOG_DEBUG, "Enabling support for engine \"%s\"", name);
    engines[current_engine]=ENGINE_by_id(name);
    if(!engines[current_engine]) {
        sslerror("ENGINE_by_id");
        return "Failed to open the engine";
    }
    engine_initialized=0;
    if(ENGINE_ctrl(engines[current_engine], ENGINE_CTRL_SET_USER_INTERFACE,
            0, ui_stunnel(), NULL)) {
        s_log(LOG_NOTICE, "UI set for engine #%d (%s)",
            current_engine+1, ENGINE_get_id(engines[current_engine]));
    } else {
        ERR_clear_error();
        s_log(LOG_INFO, "UI not supported by engine #%d (%s)",
            current_engine+1, ENGINE_get_id(engines[current_engine]));
    }
    return NULL; /* OK */
}

NOEXPORT const char *engine_ctrl(const char *cmd, const char *arg) {
    if(current_engine<0)
        return "No engine was defined";
    if(!strcasecmp(cmd, "INIT")) /* special control command */
        return engine_init();
    if(arg)
        s_log(LOG_DEBUG, "Executing engine control command %s:%s", cmd, arg);
    else
        s_log(LOG_DEBUG, "Executing engine control command %s", cmd);
    if(!ENGINE_ctrl_cmd_string(engines[current_engine], cmd, arg, 0)) {
        sslerror("ENGINE_ctrl_cmd_string");
        return "Failed to execute the engine control command";
    }
    return NULL; /* OK */
}

NOEXPORT const char *engine_default(const char *list) {
    if(current_engine<0)
        return "No engine was defined";
    if(!ENGINE_set_default_string(engines[current_engine], list)) {
        sslerror("ENGINE_set_default_string");
        return "Failed to set engine as default";
    }
    s_log(LOG_INFO, "Engine #%d (%s) set as default for %s",
        current_engine+1, ENGINE_get_id(engines[current_engine]), list);
    return NULL;
}

NOEXPORT const char *engine_init(void) {
    if(engine_initialized) /* either first or already initialized */
        return NULL; /* OK */
    s_log(LOG_DEBUG, "Initializing engine #%d (%s)",
        current_engine+1, ENGINE_get_id(engines[current_engine]));
    if(!ENGINE_init(engines[current_engine])) {
        if(ERR_peek_last_error()) /* really an error */
            sslerror("ENGINE_init");
        else
            s_log(LOG_ERR, "Engine #%d (%s) not initialized",
                current_engine+1, ENGINE_get_id(engines[current_engine]));
        return "Engine initialization failed";
    }
#if 0
    /* it is a bad idea to set the engine as default for all sections */
    /* the "engine=auto" or "engineDefault" options should be used instead */
    if(!ENGINE_set_default(engines[current_engine], ENGINE_METHOD_ALL)) {
        sslerror("ENGINE_set_default");
        return "Selecting default engine failed";
    }
#endif

    s_log(LOG_INFO, "Engine #%d (%s) initialized",
        current_engine+1, ENGINE_get_id(engines[current_engine]));
    engine_initialized=1;
    return NULL; /* OK */
}

NOEXPORT ENGINE *engine_get_by_id(const char *id) {
    int i;

    for(i=0; i<=current_engine; ++i)
        if(!strcmp(id, ENGINE_get_id(engines[i])))
            return engines[i];
    return NULL;
}

NOEXPORT ENGINE *engine_get_by_num(const int i) {
    if(i<0 || i>current_engine)
        return NULL;
    return engines[i];
}

#endif /* !defined(OPENSSL_NO_ENGINE) */

/**************************************** include config directory */

NOEXPORT const char *include_config(char *directory, SERVICE_OPTIONS **section_ptr) {
    struct dirent **namelist;
    int i, num, err=0;

    num=scandir(directory, &namelist, NULL, alphasort);
    if(num<0) {
        ioerror("scandir");
        return "Failed to include directory";
    }
    for(i=0; i<num; ++i) {
        if(!err) {
            struct stat sb;
            char *name=str_printf(
#ifdef USE_WIN32
                "%s\\%s",
#else
                "%s/%s",
#endif
                directory, namelist[i]->d_name);
            if(!stat(name, &sb) && S_ISREG(sb.st_mode))
                err=options_file(name, CONF_FILE, section_ptr);
            else
                s_log(LOG_DEBUG, "\"%s\" is not a file", name);
            str_free(name);
        }
        free(namelist[i]);
    }
    free(namelist);
    if(err)
        return "Failed to include a file";
    return NULL;
}

/**************************************** fatal error */

NOEXPORT void print_syntax(void) {
    s_log(LOG_NOTICE, " ");
    s_log(LOG_NOTICE, "Syntax:");
    s_log(LOG_NOTICE, "stunnel "
#ifdef USE_WIN32
#ifndef _WIN32_WCE
        "[ [-install | -uninstall | -start | -stop | -reload | -reopen | -exit] "
#endif
        "[-quiet] "
#endif
        "[<filename>] ] "
#ifndef USE_WIN32
        "-fd <n> "
#endif
        "| -help | -version | -sockets | -options");
    s_log(LOG_NOTICE, "    <filename>  - use specified config file");
#ifdef USE_WIN32
#ifndef _WIN32_WCE
    s_log(LOG_NOTICE, "    -install    - install NT service");
    s_log(LOG_NOTICE, "    -uninstall  - uninstall NT service");
    s_log(LOG_NOTICE, "    -start      - start NT service");
    s_log(LOG_NOTICE, "    -stop       - stop NT service");
    s_log(LOG_NOTICE, "    -reload     - reload configuration for NT service");
    s_log(LOG_NOTICE, "    -reopen     - reopen log file for NT service");
    s_log(LOG_NOTICE, "    -exit       - exit an already started stunnel");
#endif
    s_log(LOG_NOTICE, "    -quiet      - don't display message boxes");
#else
    s_log(LOG_NOTICE, "    -fd <n>     - read the config file from a file descriptor");
#endif
    s_log(LOG_NOTICE, "    -help       - get config file help");
    s_log(LOG_NOTICE, "    -version    - display version and defaults");
    s_log(LOG_NOTICE, "    -sockets    - display default socket options");
    s_log(LOG_NOTICE, "    -options    - display supported TLS options");
}

/**************************************** various supporting functions */

NOEXPORT void name_list_append(NAME_LIST **ptr, char *name) {
    while(*ptr) /* find the null pointer */
        ptr=&(*ptr)->next;
    *ptr=str_alloc_detached(sizeof(NAME_LIST));
    (*ptr)->name=str_dup_detached(name);
    (*ptr)->next=NULL;
}

NOEXPORT void name_list_dup(NAME_LIST **dst, NAME_LIST *src) {
    for(; src; src=src->next)
        name_list_append(dst, src->name);
}

NOEXPORT void name_list_free(NAME_LIST *ptr) {
    while(ptr) {
        NAME_LIST *next=ptr->next;
        str_free(ptr->name);
        str_free(ptr);
        ptr=next;
    }
}

#ifndef USE_WIN32

/* allocate 'exec' arguments */
/* TODO: support quotes */
NOEXPORT char **arg_alloc(char *str) {
    size_t max_arg, i;
    char **tmp, **retval;

    max_arg=strlen(str)/2+1;
    tmp=str_alloc((max_arg+1)*sizeof(char *));

    i=0;
    while(*str && i<max_arg) {
        tmp[i++]=str;
        while(*str && !isspace((unsigned char)*str))
            ++str;
        while(*str && isspace((unsigned char)*str))
            *str++='\0';
    }
    tmp[i]=NULL; /* null-terminate the table */

    retval=arg_dup(tmp);
    str_free(tmp);
    return retval;
}

NOEXPORT char **arg_dup(char **src) {
    size_t i, n;
    char **dst;

    if(!src)
        return NULL;
    for(n=0; src[n]; ++n)
        ;
    dst=str_alloc_detached((n+1)*sizeof(char *));
    for(i=0; i<n; ++i)
        dst[i]=str_dup_detached(src[i]);
    dst[n]=NULL;
    return dst;
}

NOEXPORT void arg_free(char **arg) {
    size_t i;

    if(arg) {
        for(i=0; arg[i]; ++i)
            str_free(arg[i]);
        str_free(arg);
    }
}

#endif

/* end of options.c */
