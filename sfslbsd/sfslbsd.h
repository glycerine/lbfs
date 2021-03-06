/*
 *
 * Copyright (C) 1998 David Mazieres (dm@uun.org)
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

#ifndef SFSRWSD_H
#define SFSRWSD_H

#include "qhash.h"
#include "arpc.h"
#include "vec.h"
#include "getfh3.h"
#include "sfscrypt.h"
#include "sfsmisc.h"
#include "mount_prot.h"
#include "crypt.h"
#include "rabin.h"
#include "seqno.h"
#include "nfstrans.h"
#include "sfsserv.h"
#include "lbfs_prot.h"
#include "lbfs.h"
#include "lbfsdb.h"
#include "fingerprint.h"
#include "axprt_compress.h"

#define FATTR3 fattr3exp

inline bool
operator== (const nfs_fh3 &a, const nfs_fh3 &b)
{
  return a.data.size () == b.data.size ()
    && !memcmp (a.data.base (), b.data.base (), b.data.size ());
}
inline bool
operator!= (const nfs_fh3 &a, const nfs_fh3 &b)
{
  return !(a == b);
}

struct hashfh3 {
  hashfh3 () {}
  hash_t operator() (const nfs_fh3 &fh) const {
    const u_int32_t *s = reinterpret_cast<const u_int32_t *> (fh.data.base ());
    const u_int32_t *e = s + (fh.data.size () >> 2);
    u_int32_t val = 0;
    while (s < e)
      val ^= *s++;
    return val;
  }
};

//
// per user fd to fh, name, and chunk translation
//

struct ufd_rec {
  unsigned fd;
  int srv_fd;
  nfs_fh3 fh;
  nfs_fh3 dir;
  bool inuse;
  bool error;
#define TMPFN_MAX 1024
  short writes;
  char name[TMPFN_MAX];
  int len;
  vec<svccb*> sbps;

  ihash_entry<ufd_rec> hlink;

  ufd_rec (unsigned d);
  ~ufd_rec ();
  void use (const nfs_fh3 &fh, const nfs_fh3 &dir, 
            const char *s, unsigned l, int sfd);
};

struct ufd_table {
  ihash<const unsigned, ufd_rec, &ufd_rec::fd, &ufd_rec::hlink> tab;
};

struct filesys {
  str host;
  ptr<aclnt> c;

  filesys *parent;
  str path_root;                // Local path corresponding to root
  str path_mntpt;               // Mountpoint relative to exported namespace
  nfs_fh3 fh_root;              // NFS File handle of root
  nfs_fh3 fh_mntpt;             // NFS File handle of mount point
  u_int64_t fsid;               // fsid of root
  u_int64_t fileid_root;        // fileid of root
  u_int64_t fileid_root_dd;     // fileid of root/..
  u_int64_t fileid_mntpt;       // fileid of mntpt
  u_int64_t fileid_mntpt_dd;    // fileid of mntpt/..
  enum {
    ANON_READ = 1,
    ANON_READWRITE = 3,
  };
  u_int options;                // Any of the above options
  ihash_entry<filesys> mphl;

  typedef qhash<u_int64_t, u_int64_t> inotab_t;
  typedef ihash<nfs_fh3, filesys, &filesys::fh_mntpt,
                &filesys::mphl, hashfh3> mp3tab_t;

  mp3tab_t &mp3tab;
  inotab_t *inotab;

  filesys () : mp3tab (*New mp3tab_t), inotab (New inotab_t) {}
};

#define SFS_TRASH_DIR_BUCKETS   254 // number of buckets (256-2, for . and ..)
#define SFS_TRASH_DIR_SIZE    64516 // total trash files (254 in each bucket)
#define SFS_TRASH_WIN_SIZE      100 // empty slots to nfs3_link to

struct trash_dir {
  nfs_fh3  topdir;
  nfs_fh3  subdirs[SFS_TRASH_DIR_BUCKETS];
  bool used[SFS_TRASH_DIR_SIZE];
  int window[SFS_TRASH_WIN_SIZE+1];
  unsigned nactive;
};

class erraccum;
struct synctab;
class filesrv {
public:
  struct reqstate {
    ptr<aclnt> c;
    u_int32_t fsno;
    bool rootfh;
  };

  ptr<sfs_servinfo_w> siw;
  sfs_servinfo servinfo;
  sfs_hash hostid;
  ptr<sfspriv> privkey;

  ptr<axprt_stream> authxprt;
  ptr<aclnt> authclnt;

  vec<filesys> fstab;
  vec<struct trash_dir> sfs_trash;
  vec<nfs_fh3> removed_fhs;
  int get_trashent(unsigned fsno);
  void clear_trashent(unsigned fsno, int srv_fd);
  void update_trashent(unsigned fsno);

  blowfish fhkey;
  sfs_fsinfo fsinfo;
  u_int leasetime;

private:
  typedef callback<void, bool>::ref cb_t;
  cb_t::ptr cb;
  struct getattr_state {
    int nleft;
    bool ok;
  };

  PRIVDEST ~filesrv ();		// No deleting

  /* Initialization functions */
  int path2fsidx (str path, size_t nfs);
  void getnfsc (ptr<aclnt> c, clnt_stat stat);
  void getmountc (ptr<aclnt> c, clnt_stat stat);

  void gotroot (ref<erraccum> ea, int i, ptr<nfsinfo> ni, str err);
  void gotroots (bool ok);

  void gottrashdir (ref<erraccum> ea, int i, int j, bool root,
		    const nfs_fh3 *fhp, str err);
  void gotrootattr (ref<erraccum> ea, int i,
		    const nfs_fh3 *fhp, const FATTR3 *attr, str err);
  void gotmp (ref<erraccum> ea, int i,
	      const nfs_fh3 *fhp, const FATTR3 *attr, str err);
  void gotmps (bool ok);

  void gotrdres (ref<erraccum>, ref<readdir3res> res,
		 int i, bool mp, clnt_stat stat);
  void gotdds (bool ok);

  void fixrdres (void *res, filesys *fsp, bool rootfh);
  void fixrdplusres (void *res, filesys *fsp, bool rootfh);

  int fhsubst (bool *substp, filesys *pfsp, nfs_fh3 *fhp, u_int32_t *fhnop);
  size_t getfsno (const filesys *fsp) const {
#ifdef CHECK_BOUNDS
    assert (fstab.base () <= fsp && fsp < fstab.lim ());
#endif CHECK_BOUNDS
    return fsp - fstab.base ();
  }

  void make_trashent(unsigned fsno, unsigned trash_idx);
  void make_trashent_lookup_cb(unsigned, unsigned, 
                               lookup3res *res, clnt_stat err);
  void make_trashent_remove_cb(wccstat3 *res, clnt_stat err);
  void db_gc(fp_db::iterator *);
  bool db_is_dirty;

public:
  synctab *const st;

  void init (cb_t cb);

  bool fixarg (svccb *sbp, reqstate *fsnop);
  bool fixres (svccb *sbp, void *res, reqstate *fsnop);

  bool getauthclnt ();

  filesrv ();
  
  fp_db fpdb;
  void db_dirty();
};

extern int sfssfd;

extern AUTH *auth_root;
extern AUTH *auth_default;

const strbuf &strbuf_cat (const strbuf &, mountstat3);
void getfh3 (const char *host, str path,
	     callback<void, const nfs_fh3 *, str>::ref);
void getfh3 (ref<aclnt> c, str path,
	     callback<void, const nfs_fh3 *, str>::ref);

// issues READ requests to server, in order. for each successful read, pass
// data pointer, number of bytes read, and offset to the rcb. when all read
// requests are finished, call cb and pass the total number of bytes read.

void nfs3_read (ref<aclnt> c, AUTH *auth, const nfs_fh3 &fh,
                off_t pos, size_t count,
                callback<void, const unsigned char *, size_t, off_t>::ref rcb,
                callback<void, size_t, read3res *, str>::ref cb);

// make nfs directory
void nfs3_mkdir (ref<aclnt> c, const nfs_fh3 &dir, const str &name, sattr3 attr,
                 callback<void, const nfs_fh3 *, str>::ref);

// copy data from one filehandle to another. for every successful read from
// the src file handle, call rcb and pass in the data pointer, number of bytes
// read, and offset. when copy is completed, call cb, pass in the file
// attribute of the dst filehandle, and the final commit res object.
void nfs3_copy (ref<aclnt> c, AUTH *auth,
                const nfs_fh3 &src, const nfs_fh3 &dst, 
                callback<void, const unsigned char *, size_t, off_t>::ref rcb,
                callback<void, commit3res *, str>::ref cb,
		bool in_order = true);

// issues multiple concurrent NFS write requests to server.
void nfs3_write (ref<aclnt> c, AUTH *auth, const nfs_fh3 &fh, 
                 callback<void, write3res *, str>::ref cb,
		 unsigned char *data, off_t pos, uint32 count, stable_how s);


class client : public virtual refcount, public sfsserv {
  filesrv *fsrv;

  ptr<asrv> nfssrv;
  ufd_table ufdtab;

  static u_int64_t nextgen ();

  void fail ();

  void nfs3dispatch (svccb *);
  void nfs3reply (svccb *sbp, void *res, filesrv::reqstate rqs, clnt_stat err);

  void renamecb_1 (svccb *sbp, void *res, filesrv::reqstate rqs,
                   clnt_stat err);
  void renamecb_2 (svccb *sbp, rename3res *rres, filesrv::reqstate rqs,
		   lookup3res *ares, clnt_stat err);

  void demux (svccb *, filesrv::reqstate rqs);
  void normal_demux (svccb *, filesrv::reqstate rqs);
  
  void trashent_link (svccb *sbp, filesrv::reqstate rqs, nfs_fh3 fh);
  void trashent_link_cb (svccb *sbp, filesrv::reqstate rqs,
                         link3res *lnres, clnt_stat err);
  void trashent_lookup_cb (svccb *sbp, filesrv::reqstate rqs,
                           lookup3res *, clnt_stat err);

  void condwrite_write_cb (svccb *sbp, filesrv::reqstate rqs, size_t count,
                           write3res *, str err);
  void condwrite_got_chunk (svccb *sbp, filesrv::reqstate rqs,
		            fp_db::iterator * iter, Chunker*,
			    unsigned char *data, 
			    size_t count, read3res *, str err);
  void condwrite_read_cb (unsigned char *, off_t, Chunker*,
                          const unsigned char *, size_t, off_t);
  void condwrite (svccb *sbp, filesrv::reqstate rqs);

  void tmpwrite_cb (svccb *sbp, filesrv::reqstate rqs,
                    write3res *wres, clnt_stat err);
  void tmpwrite (svccb *sbp, filesrv::reqstate rqs);

  void mktmpfile_cb (svccb *sbp, filesrv::reqstate rqs, nfs_fh3 dir, 
                     int sfd, void *_cres, clnt_stat err);
  void mktmpfile (svccb *sbp, filesrv::reqstate rqs);
  
  void chunk_data (Chunker *, const unsigned char *data, 
                   size_t count, off_t pos);
  void movetmp_cb (rename3res *res, clnt_stat err);
  void removetmp_cb (wccstat3 *, clnt_stat err);
  void committmp_cb (svccb *sbp, filesrv::reqstate rqs,
                     commit3res *res, str err);
  void committmp (svccb *sbp, filesrv::reqstate rqs);
  
  void aborttmp (svccb *sbp, filesrv::reqstate rqs);
 
  void getfp_cb (svccb *sbp, filesrv::reqstate rqs, Chunker *, 
                 size_t count, read3res *, str err);
  void getfp (svccb *sbp, filesrv::reqstate rqs);

protected:
  explicit client (ref<axprt_zcrypt> x);
  ~client ();
  ptr<sfspriv> doconnect (const sfs_connectarg *, sfs_servinfo *);

public:
  ptr<aclnt> nfscbc;
  const u_int64_t generation;
  ihash_entry<client> glink;

  void sfs_getfsinfo (svccb *sbp);

  static void launch (ref<axprt_zcrypt> x);
  filesrv *getfilesrv () const { return fsrv; }
};

extern ihash<const u_int64_t, client,
  &client::generation, &client::glink> clienttab;

synctab *synctab_alloc ();
void synctab_free (synctab *st);
void dolease (filesrv *fsrv, u_int64_t cgen, u_int32_t fsno, xattr *xp);
void doleases (filesrv *fsrv, u_int64_t cgen, u_int32_t fsno,
	       svccb *sbp, void *res);

bool fh3tosfs (nfs_fh3 *);
bool fh3tonfs (nfs_fh3 *);

ptr<axprt_crypt> client_accept (ptr<axprt_crypt> x);

extern filesrv *defsrv;

template<class T> inline str
stat2str (T xstat, clnt_stat stat)
{
  if (stat)
    return strbuf () << stat;
  else if (xstat)
    return strbuf () << xstat;
  else
    return NULL;
}

extern void lbfs_nfs3exp_err (svccb *sbp, nfsstat3 status);
extern void lbfs_exp_enable(u_int32_t, void *);
extern void lbfs_exp_disable(u_int32_t, void *);

#endif

