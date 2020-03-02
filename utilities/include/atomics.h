
/*******************************************************************************
 |    atomics.h
 |
 |  © Copyright IBM Corporation 2020,2020. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/

#ifdef __powerpc64__

inline void ppc_msync(void) { asm volatile ("msync" : : : "memory"); }

// Load Reserved: 64bit atom                                                                                            
inline unsigned long LoadReserved( volatile unsigned long *pVar )
{
   register unsigned long Val;
   asm volatile ("ldarx   %[rc],0,%[pVar];"
                 : [rc] "=&b" (Val)
                 : [pVar] "b" (pVar));
   return( Val );
}

// Store Conditional: 64bit atom                                                                                        
//   Returns: 0 = Conditional Store Failed,                                                                             
//            1 = Conditional Store Succeeded.                                                                          
inline int StoreConditional( volatile unsigned long *pVar, unsigned long Val )
{
   register int rc = 1; // assume success                                                                               
   asm volatile ("  stdcx.  %2,0,%1;"
                 "  beq     1f;"       // conditional store succeeded                                                   
                 "  li      %0,0;"
                 "1:;"
                 : "=b" (rc)
                 : "b"  (pVar),
                   "b"  (Val),
                   "0"  (rc)
                 : "cc", "memory" );
   return( rc );
}

// returns the orginal value of the atom when the atomic addition has succeeded                                         
inline unsigned long Fetch_and_Add( unsigned long *pVar, unsigned long value )
{
    register unsigned long old_val, tmp_val;
    ppc_msync();
    do
    {
        old_val = LoadReserved( pVar );

        tmp_val = old_val + value;
    }
    while ( !StoreConditional( pVar, tmp_val ) );
    return( old_val );
}

#else

// other architectures

#endif
