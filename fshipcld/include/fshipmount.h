/*******************************************************************************
 |    fshipmount.h
 |
 |  © Copyright IBM Corporation 2015,2016. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/
//!
//! \file   fshipmount.h
//! \author Mike Aho <meaho@us.ibm.com>
//! \date   Fri Nov 18 15:37:37 2016
//! 
//! \brief  Mount a  directory associated with open of /dev/fuse for fuse module interaction
//! 
//! 
//!

#ifndef FSHIPMOUNT_H
#define FSHIPMOUNT_H

//! \brief brief mounting a file system locally for function-shipping
//! \param target 
//! \param pMax_read 
//!
//! \return 
//!
extern int mount_fship(char *target, int pMax_read);

#endif // FSHIPMOUNT_H
