

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0622 */
/* at Mon Jan 18 22:14:07 2038
 */
/* Compiler settings for ws_proxy_stub.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 8.01.0622 
    protocol : all , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */



/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 500
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __ws_proxy_stub_h_h__
#define __ws_proxy_stub_h_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IAuthHelper_FWD_DEFINED__
#define __IAuthHelper_FWD_DEFINED__
typedef interface IAuthHelper IAuthHelper;

#endif 	/* __IAuthHelper_FWD_DEFINED__ */


#ifndef __CMyThing_FWD_DEFINED__
#define __CMyThing_FWD_DEFINED__

#ifdef __cplusplus
typedef class CMyThing CMyThing;
#else
typedef struct CMyThing CMyThing;
#endif /* __cplusplus */

#endif 	/* __CMyThing_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IAuthHelper_INTERFACE_DEFINED__
#define __IAuthHelper_INTERFACE_DEFINED__

/* interface IAuthHelper */
/* [nonextensible][oleautomation][dual][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IAuthHelper;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C7160B73-174A-4559-89B5-F1E99BA45F1B")
    IAuthHelper : public IUnknown
    {
    public:
    };
    
    
#else 	/* C style interface */

    typedef struct IAuthHelperVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAuthHelper * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAuthHelper * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAuthHelper * This);
        
        END_INTERFACE
    } IAuthHelperVtbl;

    interface IAuthHelper
    {
        CONST_VTBL struct IAuthHelperVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAuthHelper_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IAuthHelper_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IAuthHelper_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IAuthHelper_INTERFACE_DEFINED__ */



#ifndef __MyLib_LIBRARY_DEFINED__
#define __MyLib_LIBRARY_DEFINED__

/* library MyLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_MyLib;

EXTERN_C const CLSID CLSID_CMyThing;

#ifdef __cplusplus

class DECLSPEC_UUID("B8E661E9-A6D5-463D-9EF3-0434D51AEA3B")
CMyThing;
#endif
#endif /* __MyLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


