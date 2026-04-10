#include "libevsig/errno_signals.h"
#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>

SIG_DEFTYPE(SIGNAL_E2BIG);
SIG_DEFTYPE(SIGNAL_EACCES);
SIG_DEFTYPE(SIGNAL_EADDRINUSE);
SIG_DEFTYPE(SIGNAL_EADDRNOTAVAIL);
SIG_DEFTYPE(SIGNAL_EADV);
SIG_DEFTYPE(SIGNAL_EAFNOSUPPORT);
SIG_DEFTYPE(SIGNAL_EAGAIN);
SIG_DEFTYPE(SIGNAL_EALREADY);
SIG_DEFTYPE(SIGNAL_EBADE);
SIG_DEFTYPE(SIGNAL_EBADF);
SIG_DEFTYPE(SIGNAL_EBADFD);
SIG_DEFTYPE(SIGNAL_EBADMSG);
SIG_DEFTYPE(SIGNAL_EBADR);
SIG_DEFTYPE(SIGNAL_EBADRQC);
SIG_DEFTYPE(SIGNAL_EBADSLT);
SIG_DEFTYPE(SIGNAL_EBFONT);
SIG_DEFTYPE(SIGNAL_EBUSY);
SIG_DEFTYPE(SIGNAL_ECANCELED);
SIG_DEFTYPE(SIGNAL_ECHILD);
SIG_DEFTYPE(SIGNAL_ECHRNG);
SIG_DEFTYPE(SIGNAL_ECOMM);
SIG_DEFTYPE(SIGNAL_ECONNABORTED);
SIG_DEFTYPE(SIGNAL_ECONNREFUSED);
SIG_DEFTYPE(SIGNAL_ECONNRESET);
SIG_DEFTYPE(SIGNAL_EDEADLK);
SIG_DEFTYPE(SIGNAL_EDEADLOCK);
SIG_DEFTYPE(SIGNAL_EDESTADDRREQ);
SIG_DEFTYPE(SIGNAL_EDOM);
SIG_DEFTYPE(SIGNAL_EDOTDOT);
SIG_DEFTYPE(SIGNAL_EDQUOT);
SIG_DEFTYPE(SIGNAL_EEXIST);
SIG_DEFTYPE(SIGNAL_EFAULT);
SIG_DEFTYPE(SIGNAL_EFBIG);
SIG_DEFTYPE(SIGNAL_EHOSTDOWN);
SIG_DEFTYPE(SIGNAL_EHOSTUNREACH);
SIG_DEFTYPE(SIGNAL_EHWPOISON);
SIG_DEFTYPE(SIGNAL_EIDRM);
SIG_DEFTYPE(SIGNAL_EILSEQ);
SIG_DEFTYPE(SIGNAL_EINPROGRESS);
SIG_DEFTYPE(SIGNAL_EINTR);
SIG_DEFTYPE(SIGNAL_EINVAL);
SIG_DEFTYPE(SIGNAL_EIO);
SIG_DEFTYPE(SIGNAL_EISCONN);
SIG_DEFTYPE(SIGNAL_EISDIR);
SIG_DEFTYPE(SIGNAL_EISNAM);
SIG_DEFTYPE(SIGNAL_EKEYEXPIRED);
SIG_DEFTYPE(SIGNAL_EKEYREJECTED);
SIG_DEFTYPE(SIGNAL_EKEYREVOKED);
SIG_DEFTYPE(SIGNAL_EL2HLT);
SIG_DEFTYPE(SIGNAL_EL2NSYNC);
SIG_DEFTYPE(SIGNAL_EL3HLT);
SIG_DEFTYPE(SIGNAL_EL3RST);
SIG_DEFTYPE(SIGNAL_ELIBACC);
SIG_DEFTYPE(SIGNAL_ELIBBAD);
SIG_DEFTYPE(SIGNAL_ELIBEXEC);
SIG_DEFTYPE(SIGNAL_ELIBMAX);
SIG_DEFTYPE(SIGNAL_ELIBSCN);
SIG_DEFTYPE(SIGNAL_ELNRNG);
SIG_DEFTYPE(SIGNAL_ELOOP);
SIG_DEFTYPE(SIGNAL_EMEDIUMTYPE);
SIG_DEFTYPE(SIGNAL_EMFILE);
SIG_DEFTYPE(SIGNAL_EMLINK);
SIG_DEFTYPE(SIGNAL_EMSGSIZE);
SIG_DEFTYPE(SIGNAL_EMULTIHOP);
SIG_DEFTYPE(SIGNAL_ENAMETOOLONG);
SIG_DEFTYPE(SIGNAL_ENAVAIL);
SIG_DEFTYPE(SIGNAL_ENETDOWN);
SIG_DEFTYPE(SIGNAL_ENETRESET);
SIG_DEFTYPE(SIGNAL_ENETUNREACH);
SIG_DEFTYPE(SIGNAL_ENFILE);
SIG_DEFTYPE(SIGNAL_ENOANO);
SIG_DEFTYPE(SIGNAL_ENOBUFS);
SIG_DEFTYPE(SIGNAL_ENOCSI);
SIG_DEFTYPE(SIGNAL_ENODATA);
SIG_DEFTYPE(SIGNAL_ENODEV);
SIG_DEFTYPE(SIGNAL_ENOENT);
SIG_DEFTYPE(SIGNAL_ENOEXEC);
SIG_DEFTYPE(SIGNAL_ENOKEY);
SIG_DEFTYPE(SIGNAL_ENOLCK);
SIG_DEFTYPE(SIGNAL_ENOLINK);
SIG_DEFTYPE(SIGNAL_ENOMEDIUM);
SIG_DEFTYPE(SIGNAL_ENOMEM);
SIG_DEFTYPE(SIGNAL_ENOMSG);
SIG_DEFTYPE(SIGNAL_ENONET);
SIG_DEFTYPE(SIGNAL_ENOPKG);
SIG_DEFTYPE(SIGNAL_ENOPROTOOPT);
SIG_DEFTYPE(SIGNAL_ENOSPC);
SIG_DEFTYPE(SIGNAL_ENOSR);
SIG_DEFTYPE(SIGNAL_ENOSTR);
SIG_DEFTYPE(SIGNAL_ENOSYS);
SIG_DEFTYPE(SIGNAL_ENOTBLK);
SIG_DEFTYPE(SIGNAL_ENOTCONN);
SIG_DEFTYPE(SIGNAL_ENOTDIR);
SIG_DEFTYPE(SIGNAL_ENOTEMPTY);
SIG_DEFTYPE(SIGNAL_ENOTNAM);
SIG_DEFTYPE(SIGNAL_ENOTRECOVERABLE);
SIG_DEFTYPE(SIGNAL_ENOTSOCK);
SIG_DEFTYPE(SIGNAL_ENOTSUP);
SIG_DEFTYPE(SIGNAL_ENOTTY);
SIG_DEFTYPE(SIGNAL_ENOTUNIQ);
SIG_DEFTYPE(SIGNAL_ENXIO);
SIG_DEFTYPE(SIGNAL_EOF);
SIG_DEFTYPE(SIGNAL_EOPNOTSUPP);
SIG_DEFTYPE(SIGNAL_EOVERFLOW);
SIG_DEFTYPE(SIGNAL_EOWNERDEAD);
SIG_DEFTYPE(SIGNAL_EPERM);
SIG_DEFTYPE(SIGNAL_EPFNOSUPPORT);
SIG_DEFTYPE(SIGNAL_EPIPE);
SIG_DEFTYPE(SIGNAL_EPROTO);
SIG_DEFTYPE(SIGNAL_EPROTONOSUPPORT);
SIG_DEFTYPE(SIGNAL_EPROTOTYPE);
SIG_DEFTYPE(SIGNAL_ERANGE);
SIG_DEFTYPE(SIGNAL_EREMCHG);
SIG_DEFTYPE(SIGNAL_EREMOTE);
SIG_DEFTYPE(SIGNAL_EREMOTEIO);
SIG_DEFTYPE(SIGNAL_ERESTART);
SIG_DEFTYPE(SIGNAL_ERFKILL);
SIG_DEFTYPE(SIGNAL_EROFS);
SIG_DEFTYPE(SIGNAL_ESHUTDOWN);
SIG_DEFTYPE(SIGNAL_ESOCKTNOSUPPORT);
SIG_DEFTYPE(SIGNAL_ESPIPE);
SIG_DEFTYPE(SIGNAL_ESRCH);
SIG_DEFTYPE(SIGNAL_ESRMNT);
SIG_DEFTYPE(SIGNAL_ESTALE);
SIG_DEFTYPE(SIGNAL_ESTRPIPE);
SIG_DEFTYPE(SIGNAL_ETIME);
SIG_DEFTYPE(SIGNAL_ETIMEDOUT);
SIG_DEFTYPE(SIGNAL_ETOOMANYREFS);
SIG_DEFTYPE(SIGNAL_ETXTBSY);
SIG_DEFTYPE(SIGNAL_EUCLEAN);
SIG_DEFTYPE(SIGNAL_EUNATCH);
SIG_DEFTYPE(SIGNAL_EUSERS);
SIG_DEFTYPE(SIGNAL_EWOULDBLOCK);
SIG_DEFTYPE(SIGNAL_EXDEV);
SIG_DEFTYPE(SIGNAL_EXFULL);

// TODO run through aliases and make sure we only decl/define one of the
// two signal types so user is not confused and handle the wrong one

const char* sig_from_errno(int errno_in) {
  switch (errno) {
    case E2BIG:
      return SIGNAL_E2BIG;
      break;
    case EACCES:
      return SIGNAL_EACCES;
      break;
    case EADDRINUSE:
      return SIGNAL_EADDRINUSE;
      break;
    case EADDRNOTAVAIL:
      return SIGNAL_EADDRNOTAVAIL;
      break;
    case EADV:
      return SIGNAL_EADV;
      break;
    case EAFNOSUPPORT:
      return SIGNAL_EAFNOSUPPORT;
      break;
    case EALREADY:
      return SIGNAL_EALREADY;
      break;
    case EBADE:
      return SIGNAL_EBADE;
      break;
    case EBADF:
      return SIGNAL_EBADF;
      break;
    case EBADFD:
      return SIGNAL_EBADFD;
      break;
    case EBADMSG:
      return SIGNAL_EBADMSG;
      break;
    case EBADR:
      return SIGNAL_EBADR;
      break;
    case EBADRQC:
      return SIGNAL_EBADRQC;
      break;
    case EBADSLT:
      return SIGNAL_EBADSLT;
      break;
    case EBFONT:
      return SIGNAL_EBFONT;
      break;
    case EBUSY:
      return SIGNAL_EBUSY;
      break;
    case ECANCELED:
      return SIGNAL_ECANCELED;
      break;
    case ECHILD:
      return SIGNAL_ECHILD;
      break;
    case ECHRNG:
      return SIGNAL_ECHRNG;
      break;
    case ECOMM:
      return SIGNAL_ECOMM;
      break;
    case ECONNABORTED:
      return SIGNAL_ECONNABORTED;
      break;
    case ECONNREFUSED:
      return SIGNAL_ECONNREFUSED;
      break;
    case ECONNRESET:
      return SIGNAL_ECONNRESET;
      break;
    case EDEADLK: // == EDEADLOCK
      return SIGNAL_EDEADLOCK;
      break;
    case EDESTADDRREQ:
      return SIGNAL_EDESTADDRREQ;
      break;
    case EDOM:
      return SIGNAL_EDOM;
      break;
    case EDOTDOT:
      return SIGNAL_EDOTDOT;
      break;
    case EDQUOT:
      return SIGNAL_EDQUOT;
      break;
    case EEXIST:
      return SIGNAL_EEXIST;
      break;
    case EFAULT:
      return SIGNAL_EFAULT;
      break;
    case EFBIG:
      return SIGNAL_EFBIG;
      break;
    case EHOSTDOWN:
      return SIGNAL_EHOSTDOWN;
      break;
    case EHOSTUNREACH:
      return SIGNAL_EHOSTUNREACH;
      break;
    case EHWPOISON:
      return SIGNAL_EHWPOISON;
      break;
    case EIDRM:
      return SIGNAL_EIDRM;
      break;
    case EILSEQ:
      return SIGNAL_EILSEQ;
      break;
    case EINPROGRESS:
      return SIGNAL_EINPROGRESS;
      break;
    case EINTR:
      return SIGNAL_EINTR;
      break;
    case EINVAL:
      return SIGNAL_EINVAL;
      break;
    case EIO:
      return SIGNAL_EIO;
      break;
    case EISCONN:
      return SIGNAL_EISCONN;
      break;
    case EISDIR:
      return SIGNAL_EISDIR;
      break;
    case EISNAM:
      return SIGNAL_EISNAM;
      break;
    case EKEYEXPIRED:
      return SIGNAL_EKEYEXPIRED;
      break;
    case EKEYREJECTED:
      return SIGNAL_EKEYREJECTED;
      break;
    case EKEYREVOKED:
      return SIGNAL_EKEYREVOKED;
      break;
    case EL2HLT:
      return SIGNAL_EL2HLT;
      break;
    case EL2NSYNC:
      return SIGNAL_EL2NSYNC;
      break;
    case EL3HLT:
      return SIGNAL_EL3HLT;
      break;
    case EL3RST:
      return SIGNAL_EL3RST;
      break;
    case ELIBACC:
      return SIGNAL_ELIBACC;
      break;
    case ELIBBAD:
      return SIGNAL_ELIBBAD;
      break;
    case ELIBEXEC:
      return SIGNAL_ELIBEXEC;
      break;
    case ELIBMAX:
      return SIGNAL_ELIBMAX;
      break;
    case ELIBSCN:
      return SIGNAL_ELIBSCN;
      break;
    case ELNRNG:
      return SIGNAL_ELNRNG;
      break;
    case ELOOP:
      return SIGNAL_ELOOP;
      break;
    case EMEDIUMTYPE:
      return SIGNAL_EMEDIUMTYPE;
      break;
    case EMFILE:
      return SIGNAL_EMFILE;
      break;
    case EMLINK:
      return SIGNAL_EMLINK;
      break;
    case EMSGSIZE:
      return SIGNAL_EMSGSIZE;
      break;
    case EMULTIHOP:
      return SIGNAL_EMULTIHOP;
      break;
    case ENAMETOOLONG:
      return SIGNAL_ENAMETOOLONG;
      break;
    case ENAVAIL:
      return SIGNAL_ENAVAIL;
      break;
    case ENETDOWN:
      return SIGNAL_ENETDOWN;
      break;
    case ENETRESET:
      return SIGNAL_ENETRESET;
      break;
    case ENETUNREACH:
      return SIGNAL_ENETUNREACH;
      break;
    case ENFILE:
      return SIGNAL_ENFILE;
      break;
    case ENOANO:
      return SIGNAL_ENOANO;
      break;
    case ENOBUFS:
      return SIGNAL_ENOBUFS;
      break;
    case ENOCSI:
      return SIGNAL_ENOCSI;
      break;
    case ENODATA:
      return SIGNAL_ENODATA;
      break;
    case ENODEV:
      return SIGNAL_ENODEV;
      break;
    case ENOENT:
      return SIGNAL_ENOENT;
      break;
    case ENOEXEC:
      return SIGNAL_ENOEXEC;
      break;
    case ENOKEY:
      return SIGNAL_ENOKEY;
      break;
    case ENOLCK:
      return SIGNAL_ENOLCK;
      break;
    case ENOLINK:
      return SIGNAL_ENOLINK;
      break;
    case ENOMEDIUM:
      return SIGNAL_ENOMEDIUM;
      break;
    case ENOMEM:
      return SIGNAL_ENOMEM;
      break;
    case ENOMSG:
      return SIGNAL_ENOMSG;
      break;
    case ENONET:
      return SIGNAL_ENONET;
      break;
    case ENOPKG:
      return SIGNAL_ENOPKG;
      break;
    case ENOPROTOOPT:
      return SIGNAL_ENOPROTOOPT;
      break;
    case ENOSPC:
      return SIGNAL_ENOSPC;
      break;
    case ENOSR:
      return SIGNAL_ENOSR;
      break;
    case ENOSTR:
      return SIGNAL_ENOSTR;
      break;
    case ENOSYS:
      return SIGNAL_ENOSYS;
      break;
    case ENOTBLK:
      return SIGNAL_ENOTBLK;
      break;
    case ENOTCONN:
      return SIGNAL_ENOTCONN;
      break;
    case ENOTDIR:
      return SIGNAL_ENOTDIR;
      break;
    case ENOTEMPTY:
      return SIGNAL_ENOTEMPTY;
      break;
    case ENOTNAM:
      return SIGNAL_ENOTNAM;
      break;
    case ENOTRECOVERABLE:
      return SIGNAL_ENOTRECOVERABLE;
      break;
    case ENOTSOCK:
      return SIGNAL_ENOTSOCK;
      break;
    case EOPNOTSUPP: // == ENOTSUP
      return SIGNAL_EOPNOTSUPP;
      break;
    case ENOTTY:
      return SIGNAL_ENOTTY;
      break;
    case ENOTUNIQ:
      return SIGNAL_ENOTUNIQ;
      break;
    case ENXIO:
      return SIGNAL_ENXIO;
      break;
    case EOF:
      return SIGNAL_EOF;
      break;
    case EOVERFLOW:
      return SIGNAL_EOVERFLOW;
      break;
    case EOWNERDEAD:
      return SIGNAL_EOWNERDEAD;
      break;
    case EPERM:
      return SIGNAL_EPERM;
      break;
    case EPFNOSUPPORT:
      return SIGNAL_EPFNOSUPPORT;
      break;
    case EPIPE:
      return SIGNAL_EPIPE;
      break;
    case EPROTO:
      return SIGNAL_EPROTO;
      break;
    case EPROTONOSUPPORT:
      return SIGNAL_EPROTONOSUPPORT;
      break;
    case EPROTOTYPE:
      return SIGNAL_EPROTOTYPE;
      break;
    case ERANGE:
      return SIGNAL_ERANGE;
      break;
    case EREMCHG:
      return SIGNAL_EREMCHG;
      break;
    case EREMOTE:
      return SIGNAL_EREMOTE;
      break;
    case EREMOTEIO:
      return SIGNAL_EREMOTEIO;
      break;
    case ERESTART:
      return SIGNAL_ERESTART;
      break;
    case ERFKILL:
      return SIGNAL_ERFKILL;
      break;
    case EROFS:
      return SIGNAL_EROFS;
      break;
    case ESHUTDOWN:
      return SIGNAL_ESHUTDOWN;
      break;
    case ESOCKTNOSUPPORT:
      return SIGNAL_ESOCKTNOSUPPORT;
      break;
    case ESPIPE:
      return SIGNAL_ESPIPE;
      break;
    case ESRCH:
      return SIGNAL_ESRCH;
      break;
    case ESRMNT:
      return SIGNAL_ESRMNT;
      break;
    case ESTALE:
      return SIGNAL_ESTALE;
      break;
    case ESTRPIPE:
      return SIGNAL_ESTRPIPE;
      break;
    case ETIME:
      return SIGNAL_ETIME;
      break;
    case ETIMEDOUT:
      return SIGNAL_ETIMEDOUT;
      break;
    case ETOOMANYREFS:
      return SIGNAL_ETOOMANYREFS;
      break;
    case ETXTBSY:
      return SIGNAL_ETXTBSY;
      break;
    case EUCLEAN:
      return SIGNAL_EUCLEAN;
      break;
    case EUNATCH:
      return SIGNAL_EUNATCH;
      break;
    case EUSERS:
      return SIGNAL_EUSERS;
      break;
    case EAGAIN: // == EWOULDBLOCK
      return SIGNAL_EAGAIN;
      break;
    case EXDEV:
      return SIGNAL_EXDEV;
      break;
    case EXFULL:
      return SIGNAL_EXFULL;
      break;
  }

  return NULL;
}

static thread_local char prefixed_str[1024];
const char* str_from_errno(const char* prefix, int errno_in) {
  prefixed_str[0] = '\0';

  strncat(prefixed_str, prefix, 256);
  strerror_r(errno_in, prefixed_str+strlen(prefixed_str), 767);

  return prefixed_str;
}
