/* 
 * File:   sockutils.h
 * Author: keritaf
 *
 * Created on October 27, 2012, 6:46 AM
 */

#ifndef SOCKUTILS_H
#define	SOCKUTILS_H

#ifdef	__cplusplus
extern "C" {
#endif

  int mksock(char * host,
          char * service,
          char * proto,
          struct sockaddr_in * sin);
  int servsock(char * host,
               char * service,
               char * proto,
               struct sockaddr_in * sin)
#ifdef	__cplusplus
}
#endif

#endif	/* SOCKUTILS_H */

