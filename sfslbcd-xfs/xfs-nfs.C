/*
 *
 * Copyright (C) 2000 Athicha Muthitacharoen (athicha@mit.edu)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 */

#include "xfs-nfs.h"
#include "sfslbcd.h"
#include "dmalloc.h"

u_char 
nfs_rights2xfs_rights (u_int32_t access, ftype3 ftype, u_int32_t mode) 
{
  u_char ret = 0;

  if (ftype == NF3DIR) {
    if (access & (ACCESS3_READ | ACCESS3_LOOKUP | ACCESS3_EXECUTE))
      ret |= XFS_RIGHT_R | XFS_RIGHT_X;
    if (access & (ACCESS3_MODIFY | ACCESS3_EXTEND | ACCESS3_DELETE))
      ret |= XFS_RIGHT_W;
  } else {
    if ((ftype == NF3LNK) && 
	(access & (ACCESS3_READ | ACCESS3_LOOKUP | ACCESS3_EXECUTE)))
      ret |= XFS_RIGHT_R;
    if ((access & (ACCESS3_READ | ACCESS3_LOOKUP)) && (mode & S_IRUSR)) 
      ret |= XFS_RIGHT_R;
    if ((access & (ACCESS3_MODIFY | ACCESS3_EXTEND | ACCESS3_DELETE)) && 
	(mode & S_IWUSR)) 
      ret |= XFS_RIGHT_W;
    if ((access & ACCESS3_EXECUTE) && (mode & S_IXUSR)) 
      ret |= XFS_RIGHT_X;    
  }
    
  return ret;
}

void 
nfsobj2xfsnode (xfs_cred cred, cache_entry *e, xfs_msg_node *node) 
{
  node->handle = e->xh; 
  if (lbcd_trace > 1)
    warn << "nfsfh becomes node.handle (" 
	 << node->handle.a << ","
	 << node->handle.b << ","
	 << node->handle.c << ","
	 << node->handle.d << ")\n";
  

  node->anonrights = XFS_RIGHT_R | XFS_RIGHT_W | XFS_RIGHT_X;
  node->tokens = XFS_ATTR_R; // | ~XFS_DATA_MASK;

  /* node->attr */
  node->attr.valid = XA_V_NONE;
  if (e->nfs_attr.type == NF3REG) {
    XA_SET_MODE(&node->attr, S_IFREG);
    XA_SET_TYPE(&node->attr, XFS_FILE_REG);
    XA_SET_NLINK(&node->attr, e->nfs_attr.nlink);
  } else 
    if (e->nfs_attr.type == NF3DIR) {
      XA_SET_MODE(&node->attr, S_IFDIR);
      XA_SET_TYPE(&node->attr, XFS_FILE_DIR);
      XA_SET_NLINK(&node->attr, e->nfs_attr.nlink);
    } else
      if (e->nfs_attr.type == NF3LNK) {
	XA_SET_MODE(&node->attr, S_IFLNK);
	XA_SET_TYPE(&node->attr, XFS_FILE_LNK);
	XA_SET_NLINK(&node->attr, e->nfs_attr.nlink);
      } else {
	warn << "nfsattr2xfs_attr: default\n";
	abort ();
      }
  XA_SET_SIZE(&node->attr, e->nfs_attr.size);
  XA_SET_UID(&node->attr,e->nfs_attr.uid);
  XA_SET_GID(&node->attr, e->nfs_attr.gid);
  node->attr.xa_mode  |= e->nfs_attr.mode;
  XA_SET_ATIME(&node->attr, e->nfs_attr.atime.seconds);
  XA_SET_MTIME(&node->attr, e->nfs_attr.mtime.seconds);
  XA_SET_CTIME(&node->attr, e->nfs_attr.ctime.seconds);
  XA_SET_FILEID(&node->attr, e->nfs_attr.fileid);

  //HARD CODE ACCESS FOR NOW!! use nfs3_access later
  node->anonrights = nfs_rights2xfs_rights(ACCESS3_READ  | 
					   ACCESS3_LOOKUP | 
					   ACCESS3_EXECUTE |
					   ACCESS3_MODIFY | 
					   ACCESS3_EXTEND | 
					   ACCESS3_DELETE,
					   e->nfs_attr.type, 
					   e->nfs_attr.mode);

  for (int i=0; i<MAXRIGHTS; i++) {
    node->id[i] = cred.pag;
    node->rights[i] = nfs_rights2xfs_rights(ACCESS3_READ  | 
					    ACCESS3_LOOKUP |
					    ACCESS3_EXECUTE | 
					    ACCESS3_MODIFY | 
					    ACCESS3_EXTEND | 
					    ACCESS3_DELETE,
					    e->nfs_attr.type, 
					    e->nfs_attr.mode);  
  }
}

#if 0
void create_xfsnode 
(xfs_cred cred, const ex_fattr3 *nfs_attr, xfs_msg_node *node) 
{
  node->handle = e->xh; 
  if (lbcd_trace > 1)
    warn << "nfsfh becomes node.handle (" 
	 << node->handle.a << ","
	 << node->handle.b << ","
	 << node->handle.c << ","
	 << node->handle.d << ")\n";
  

  node->anonrights = XFS_RIGHT_R | XFS_RIGHT_W | XFS_RIGHT_X;
  node->tokens = XFS_ATTR_R; // | ~XFS_DATA_MASK;

  /* node->attr */
  node->attr.valid = XA_V_NONE;
  if (e->nfs_attr.type == NF3REG) {
    XA_SET_MODE(&node->attr, S_IFREG);
    XA_SET_TYPE(&node->attr, XFS_FILE_REG);
    XA_SET_NLINK(&node->attr, e->nfs_attr.nlink);
  } else 
    if (e->nfs_attr.type == NF3DIR) {
      XA_SET_MODE(&node->attr, S_IFDIR);
      XA_SET_TYPE(&node->attr, XFS_FILE_DIR);
      XA_SET_NLINK(&node->attr, e->nfs_attr.nlink);
    } else
      if (e->nfs_attr.type == NF3LNK) {
	XA_SET_MODE(&node->attr, S_IFLNK);
	XA_SET_TYPE(&node->attr, XFS_FILE_LNK);
	XA_SET_NLINK(&node->attr, e->nfs_attr.nlink);
      } else {
	warn << "nfsattr2xfs_attr: default\n";
	abort ();
      }
  XA_SET_SIZE(&node->attr, e->nfs_attr.size);
  XA_SET_UID(&node->attr,e->nfs_attr.uid);
  XA_SET_GID(&node->attr, e->nfs_attr.gid);
  node->attr.xa_mode  |= e->nfs_attr.mode;
  XA_SET_ATIME(&node->attr, e->nfs_attr.atime.seconds);
  XA_SET_MTIME(&node->attr, e->nfs_attr.mtime.seconds);
  XA_SET_CTIME(&node->attr, e->nfs_attr.ctime.seconds);
  XA_SET_FILEID(&node->attr, e->nfs_attr.fileid);

  //HARD CODE ACCESS FOR NOW!! use nfs3_access later
  node->anonrights = nfs_rights2xfs_rights(ACCESS3_READ  | 
					   ACCESS3_LOOKUP | 
					   ACCESS3_EXECUTE |
					   ACCESS3_MODIFY | 
					   ACCESS3_EXTEND | 
					   ACCESS3_DELETE,
					   e->nfs_attr.type, 
					   e->nfs_attr.mode);

  for (int i=0; i<MAXRIGHTS; i++) {
    node->id[i] = cred.pag;
    node->rights[i] = nfs_rights2xfs_rights(ACCESS3_READ  | 
					    ACCESS3_LOOKUP |
					    ACCESS3_EXECUTE | 
					    ACCESS3_MODIFY | 
					    ACCESS3_EXTEND | 
					    ACCESS3_DELETE,
					    e->nfs_attr.type, 
					    e->nfs_attr.mode);  
  }
}
#endif

static long blocksize = XFS_DIRENT_BLOCKSIZE;

int 
flushbuf(write_dirent_args *args) 
{
  unsigned inc = blocksize - (args->ptr - args->buf);
  xfs_dirent *last = (xfs_dirent *)args->last;

  last->d_reclen += inc;
  if (write (args->fd, args->buf, blocksize) != blocksize) {
    warn << "(" << errno << "):write\n";
    return -1;
  }
  args->ptr = args->buf;
  args->last = NULL;
  return 0;
}

int 
nfsdir2xfsfile(ex_readdir3res *res, write_dirent_args *args) 
{
#if 1
  args->off = 0;
  //args->buf = (char *)malloc (blocksize);
  //if (args->buf == NULL) {
  //  warn << "nfsdir2xfsfile: malloc error\n";
  //  return -1;
  //}
  args->ptr = args->buf;
  args->last = NULL;
#endif  
  assert(res->status == NFS3_OK);
  entry3 *nfs_dirent = res->resok->reply.entries;
  xfs_dirent *xde; 
  int reclen = sizeof(*xde);

  while (nfs_dirent != NULL) {
#if 1
    if (args->ptr + reclen > args->buf + blocksize) {
      if (flushbuf (args) < 0) 
	return -1;
    }
    xde = (xfs_dirent *) args->ptr;   
#endif
    bzero(xde, sizeof(*xde));
    xde->d_namlen = nfs_dirent->name.len();
    if (lbcd_trace > 1)
      warn << "xde->namlen = " << xde->d_namlen 
	   << " nfs_dirent_len = " << nfs_dirent->name.len() << "\n";
    xde->d_reclen = reclen;
#if defined(HAVE_STRUCT_DIRENT_D_TYPE) && !defined(__linux__)
    xde->d_type = DT_UNKNOWN;
#endif
    xde->d_fileno = nfs_dirent->fileid;
    strcpy(xde->d_name, nfs_dirent->name.cstr());
    if (lbcd_trace > 1)
      warn << "xde->d_name = " << xde->d_name 
	   << " nfs_dirent_name = " << nfs_dirent->name.cstr() << "\n";
#if 1
    args->ptr += xde->d_reclen;
    args->off += xde->d_reclen;
    args->last = xde;
#else    
    if (write (args->fd, xde, reclen) != reclen) {
      if (lbcd_trace > 1)
	warn << "(" << errno << "):write\n";
      return -1;
    }
    if (lbcd_trace > 1)
      warn << "wrote " << xde->d_name << "\n";
#endif
    nfs_dirent = nfs_dirent->nextentry;
  }
  return 0;
}

int 
conv_dir (int fd, ex_readdir3res *res) 
{
  assert(res->status == NFS3_OK);
  entry3 *nfs_dirent = res->resok->reply.entries;
  xfs_dirent *xde = (xfs_dirent *) malloc (sizeof (*xde)); 
  int reclen = sizeof(*xde);

  while (nfs_dirent != NULL) {
    bzero(xde, sizeof(*xde));
    xde->d_namlen = nfs_dirent->name.len();
    if (lbcd_trace > 1)
      warn << "xde->namlen = " << xde->d_namlen 
	   << " nfs_dirent_len = " << nfs_dirent->name.len() << "\n";
    xde->d_reclen = reclen;
#if defined(HAVE_STRUCT_DIRENT_D_TYPE) && !defined(__linux__)
    xde->d_type = DT_UNKNOWN;
#endif
    xde->d_fileno = nfs_dirent->fileid;
    strcpy(xde->d_name, nfs_dirent->name.cstr());
    if (lbcd_trace > 1)
      warn << "xde->d_name = " << xde->d_name 
	   << " nfs_dirent_name = " << nfs_dirent->name.cstr() << "\n";
    if (write (fd, xde, reclen) != reclen) {
      if (lbcd_trace > 1)
	warn << "(" << errno << "):write\n";
      free (xde);
      return -1;
    }
    if (lbcd_trace > 1)
      warn << "wrote " << xde->d_name << "\n";
    nfs_dirent = nfs_dirent->nextentry;
  }
  free (xde);
  return 0;
}

int 
nfsdirent2xfsfile(int fd, const char* fname, uint64 fid) 
{

  xfs_dirent *xde = (xfs_dirent *)malloc(sizeof(*xde));
  bzero(xde, sizeof(*xde));
  xde->d_namlen = strlen(fname);
  strcpy(xde->d_name, fname);
  xde->d_reclen = sizeof(*xde);
#if defined(HAVE_STRUCT_DIRENT_D_TYPE) && !defined(__linux__)
  xde->d_type = DT_UNKNOWN;
#endif
  xde->d_fileno = fid;
  
  if (write(fd, xde, xde->d_reclen) != xde->d_reclen) {
    if (lbcd_trace > 1)
      warn << strerror(errno) << "(" << errno << "):write\n";
    free (xde);
    return -1;
  }
  free (xde);
  return 0;
}

int 
xfsfile_rm_dirent(int fd1, int fd2, const char* fname) 
{
  xfs_dirent *xde = (xfs_dirent *)malloc(sizeof(*xde));
  int err, offset = 0, reclen = sizeof(*xde);
  
  do {
    if ((err = read(fd1, xde, sizeof(*xde))) < 0) {
      if (lbcd_trace > 1)
	warn << "xfsfile_rm_dirent: " << strerror(errno) << "\n";
      free (xde);
      return -1;
    }
    if (err != sizeof(*xde)) {
      if (lbcd_trace > 1)
	warn << "err = " << err << " ..short read..wierd\n";
      free (xde);
      return -1;
    }
    offset += reclen;
    if (lbcd_trace > 1) {
      warn << "xde->d_namlen = " << xde->d_namlen << "\n";
      warn << "xde->d_name = " << xde->d_name << "\n";
      warn << "xde->d_reclen = " << xde->d_reclen << "\n";
      warn << "xde->d_fileno = " << xde->d_fileno << "\n";
    }
  } while (strncmp(xde->d_name, fname, strlen(fname)));

  offset -= reclen;
  lseek(fd2, offset, SEEK_SET);

  while (read(fd1, xde, sizeof(*xde)) == sizeof (*xde)) {
    offset += reclen;
    err = write (fd2, xde, sizeof (*xde));
    if (err != sizeof(*xde)) {
      if (lbcd_trace > 1)
      warn << "err = " << err << " ..short write..wierd\n";
      free (xde);
      return -1;
    }
  }

  ftruncate (fd2, offset);
  free (xde);
  return 0;
}

#ifndef DIRBLKSIZ
#define DIRBLKSIZ 1024
#endif

int 
dir_remove_name (int fd, const char *fname) 
{
  struct stat sb;
  size_t len;

  if (fstat (fd, &sb) < 0) {
    return errno;
  }
  len = sb.st_size;
  char *buf = (char *) mmap (0, len, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
  if (buf == (char *) MAP_FAILED)
    return errno;
  char *p;
  int ret = ENOENT;
  struct xfs_dirent *dp, *last_dp = NULL;
  for (p = buf; p < buf + len; p += dp->d_reclen) {
    dp = (struct xfs_dirent *) p;
    assert (dp->d_reclen > 0);
    if (strcmp (fname, dp->d_name) == 0) {
      if (last_dp != NULL) {
	unsigned length;
	length = last_dp->d_reclen + dp->d_reclen;
	if (length < DIRBLKSIZ)
	  last_dp->d_reclen = len;
      }
      dp->d_fileno = 0;
      ret = 0;
      break;
    }
    last_dp = dp;
  }

  if (munmap (buf, len) < 0)
    return errno;
  return 0;
}

int 
xfsattr2nfsattr(uint32 opcode, xfs_attr xa, sattr3 *na) 
{

  if (XA_VALID_MODE(&xa)) {
    na->mode.set_set(true);
    *na->mode.val = xa.xa_mode;
  }

  if (opcode != XFS_MSG_CREATE) {
    if (XA_VALID_UID(&xa)) {
      na->uid.set_set(true);
      // warn << "xfs_uid = " << xa.xa_uid << "\n";
      *na->uid.val = xa.xa_uid;
    }
    
    if (XA_VALID_GID(&xa)) {
      na->gid.set_set(true);
      // warn << "xfs_gid = " << xa.xa_gid << "\n";
      *na->gid.val = xa.xa_gid;
    }
  }

  if (XA_VALID_SIZE(&xa)) {
    na->size.set_set(true);
    // warn << "xfs_size = " << xa.xa_size << "\n";
    *na->size.val = xa.xa_size;
  }

  if (XA_VALID_ATIME(&xa)) {
    na->atime.set_set(SET_TO_CLIENT_TIME);
    na->atime.time->seconds = xa.xa_atime;
    na->atime.time->nseconds = 0;
  } else 
    na->atime.set_set(SET_TO_SERVER_TIME);

  if (XA_VALID_MTIME(&xa)) {  
    na->mtime.set_set(SET_TO_CLIENT_TIME);
    na->mtime.time->seconds = xa.xa_mtime;
    na->mtime.time->nseconds = 0;
  } else 
    na->mtime.set_set(SET_TO_SERVER_TIME);

  return 0;
}

int 
fattr2sattr(ex_fattr3 fa, sattr3 *sa) 
{  
  sa->mode.set_set(true);
  *sa->mode.val = fa.mode;

  sa->uid.set_set(true);
  *sa->uid.val = fa.uid;

  sa->gid.set_set(true);
  *sa->gid.val = fa.gid;

  sa->size.set_set(true);
  *sa->size.val = fa.size;

  sa->atime.set_set(SET_TO_SERVER_TIME);

  if (sa->atime.set == SET_TO_CLIENT_TIME) {
    sa->atime.time->seconds = fa.atime.seconds;
    sa->atime.time->nseconds = fa.atime.nseconds;
  }

  sa->mtime.set_set(SET_TO_SERVER_TIME);

  if (sa->mtime.set == SET_TO_CLIENT_TIME) {
    sa->mtime.time->seconds = fa.mtime.seconds;
    sa->mtime.time->nseconds = fa.mtime.nseconds;
  }

  return 0;  
}

void 
xfs_reply_err (int fd, u_int seqnum, int err)
{
  struct xfs_message_header *h0 = NULL;
  size_t h0_len = 0;

  if (lbcd_trace > 1) {
    if (err == EIO)
      warn << seqnum << ": sending EIO to xfs\n";
    else warn << seqnum << ": " << strerror(err) << "\n";
  }
  xfs_send_message_wakeup_multiple (fd, seqnum, err, h0, h0_len, NULL, 0);
}

