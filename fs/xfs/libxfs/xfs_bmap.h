/*
 * Copyright (c) 2000-2006 Silicon Graphics, Inc.
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it would be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write the Free Software Foundation,
 * Inc.,  51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef __XFS_BMAP_H__
#define	__XFS_BMAP_H__

struct getbmap;
struct xfs_bmbt_irec;
struct xfs_ifork;
struct xfs_inode;
struct xfs_mount;
struct xfs_trans;

extern kmem_zone_t	*xfs_bmap_free_item_zone;

/*
 * Argument structure for xfs_bmap_alloc.
 */
struct xfs_bmalloca {
	xfs_fsblock_t		*firstblock; /* i/o first block allocated */
	struct xfs_defer_ops	*dfops;	/* bmap freelist */
	struct xfs_trans	*tp;	/* transaction pointer */
	struct xfs_inode	*ip;	/* incore inode pointer */
	struct xfs_bmbt_irec	prev;	/* extent before the new one */
	struct xfs_bmbt_irec	got;	/* extent after, or delayed */

	xfs_fileoff_t		offset;	/* offset in file filling in */
	xfs_extlen_t		length;	/* i/o length asked/allocated */
	xfs_fsblock_t		blkno;	/* starting block of new extent */

	struct xfs_btree_cur	*cur;	/* btree cursor */
	struct xfs_iext_cursor	icur;	/* incore extent cursor */
	int			nallocs;/* number of extents alloc'd */
	int			logflags;/* flags for transaction logging */

	xfs_extlen_t		total;	/* total blocks needed for xaction */
	xfs_extlen_t		minlen;	/* minimum allocation size (blocks) */
	xfs_extlen_t		minleft; /* amount must be left after alloc */
	bool			eof;	/* set if allocating past last extent */
	bool			wasdel;	/* replacing a delayed allocation */
	bool			aeof;	/* allocated space at eof */
	bool			conv;	/* overwriting unwritten extents */
	int			datatype;/* data type being allocated */
	int			flags;
};

/*
 * List of extents to be free "later".
 * The list is kept sorted on xbf_startblock.
 */
struct xfs_extent_free_item
{
	xfs_fsblock_t		xefi_startblock;/* starting fs block number */
	xfs_extlen_t		xefi_blockcount;/* number of blocks in extent */
	struct list_head	xefi_list;
	struct xfs_owner_info	xefi_oinfo;	/* extent owner */
	bool			xefi_skip_discard;
};

#define	XFS_BMAP_MAX_NMAP	4

/*
 * Flags for xfs_bmapi_*
 */
#define XFS_BMAPI_ENTIRE	0x001	/* return entire extent, not trimmed */
#define XFS_BMAPI_METADATA	0x002	/* mapping metadata not user data */
#define XFS_BMAPI_ATTRFORK	0x004	/* use attribute fork not data */
#define XFS_BMAPI_PREALLOC	0x008	/* preallocation op: unwritten space */
#define XFS_BMAPI_IGSTATE	0x010	/* Ignore state - */
					/* combine contig. space */
#define XFS_BMAPI_CONTIG	0x020	/* must allocate only one extent */
/*
 * unwritten extent conversion - this needs write cache flushing and no additional
 * allocation alignments. When specified with XFS_BMAPI_PREALLOC it converts
 * from written to unwritten, otherwise convert from unwritten to written.
 */
#define XFS_BMAPI_CONVERT	0x040

/*
 * allocate zeroed extents - this requires all newly allocated user data extents
 * to be initialised to zero. It will be ignored if XFS_BMAPI_METADATA is set.
 * Use in conjunction with XFS_BMAPI_CONVERT to convert unwritten extents found
 * during the allocation range to zeroed written extents.
 */
#define XFS_BMAPI_ZERO		0x080

/*
 * Map the inode offset to the block given in ap->firstblock.  Primarily
 * used for reflink.  The range must be in a hole, and this flag cannot be
 * turned on with PREALLOC or CONVERT, and cannot be used on the attr fork.
 *
 * For bunmapi, this flag unmaps the range without adjusting quota, reducing
 * refcount, or freeing the blocks.
 */
#define XFS_BMAPI_REMAP		0x100

/* Map something in the CoW fork. */
#define XFS_BMAPI_COWFORK	0x200

/* Only convert delalloc space, don't allocate entirely new extents */
#define XFS_BMAPI_DELALLOC	0x400

/* Only convert unwritten extents, don't allocate new blocks */
#define XFS_BMAPI_CONVERT_ONLY	0x800

/* Skip online discard of freed extents */
#define XFS_BMAPI_NODISCARD	0x1000

/* Do not update the rmap btree.  Used for reconstructing bmbt from rmapbt. */
#define XFS_BMAPI_NORMAP	0x2000

#define XFS_BMAPI_FLAGS \
	{ XFS_BMAPI_ENTIRE,	"ENTIRE" }, \
	{ XFS_BMAPI_METADATA,	"METADATA" }, \
	{ XFS_BMAPI_ATTRFORK,	"ATTRFORK" }, \
	{ XFS_BMAPI_PREALLOC,	"PREALLOC" }, \
	{ XFS_BMAPI_IGSTATE,	"IGSTATE" }, \
	{ XFS_BMAPI_CONTIG,	"CONTIG" }, \
	{ XFS_BMAPI_CONVERT,	"CONVERT" }, \
	{ XFS_BMAPI_ZERO,	"ZERO" }, \
	{ XFS_BMAPI_REMAP,	"REMAP" }, \
	{ XFS_BMAPI_COWFORK,	"COWFORK" }, \
	{ XFS_BMAPI_DELALLOC,	"DELALLOC" }, \
	{ XFS_BMAPI_CONVERT_ONLY, "CONVERT_ONLY" }, \
	{ XFS_BMAPI_NODISCARD,	"NODISCARD" }, \
	{ XFS_BMAPI_NORMAP,	"NORMAP" }


static inline int xfs_bmapi_aflag(int w)
{
	return (w == XFS_ATTR_FORK ? XFS_BMAPI_ATTRFORK :
	       (w == XFS_COW_FORK ? XFS_BMAPI_COWFORK : 0));
}

static inline int xfs_bmapi_whichfork(int bmapi_flags)
{
	if (bmapi_flags & XFS_BMAPI_COWFORK)
		return XFS_COW_FORK;
	else if (bmapi_flags & XFS_BMAPI_ATTRFORK)
		return XFS_ATTR_FORK;
	return XFS_DATA_FORK;
}

/*
 * Special values for xfs_bmbt_irec_t br_startblock field.
 */
#define	DELAYSTARTBLOCK		((xfs_fsblock_t)-1LL)
#define	HOLESTARTBLOCK		((xfs_fsblock_t)-2LL)

/*
 * Flags for xfs_bmap_add_extent*.
 */
#define BMAP_LEFT_CONTIG	(1 << 0)
#define BMAP_RIGHT_CONTIG	(1 << 1)
#define BMAP_LEFT_FILLING	(1 << 2)
#define BMAP_RIGHT_FILLING	(1 << 3)
#define BMAP_LEFT_DELAY		(1 << 4)
#define BMAP_RIGHT_DELAY	(1 << 5)
#define BMAP_LEFT_VALID		(1 << 6)
#define BMAP_RIGHT_VALID	(1 << 7)
#define BMAP_ATTRFORK		(1 << 8)
#define BMAP_COWFORK		(1 << 9)

#define XFS_BMAP_EXT_FLAGS \
	{ BMAP_LEFT_CONTIG,	"LC" }, \
	{ BMAP_RIGHT_CONTIG,	"RC" }, \
	{ BMAP_LEFT_FILLING,	"LF" }, \
	{ BMAP_RIGHT_FILLING,	"RF" }, \
	{ BMAP_ATTRFORK,	"ATTR" }, \
	{ BMAP_COWFORK,		"COW" }


/*
 * Return true if the extent is a real, allocated extent, or false if it is  a
 * delayed allocation, and unwritten extent or a hole.
 */
static inline bool xfs_bmap_is_real_extent(struct xfs_bmbt_irec *irec)
{
	return irec->br_state != XFS_EXT_UNWRITTEN &&
		irec->br_startblock != HOLESTARTBLOCK &&
		irec->br_startblock != DELAYSTARTBLOCK &&
		!isnullstartblock(irec->br_startblock);
}

void	xfs_trim_extent(struct xfs_bmbt_irec *irec, xfs_fileoff_t bno,
		xfs_filblks_t len);
void	xfs_trim_extent_eof(struct xfs_bmbt_irec *, struct xfs_inode *);
int	xfs_bmap_add_attrfork(struct xfs_inode *ip, int size, int rsvd);
void	xfs_bmap_local_to_extents_empty(struct xfs_inode *ip, int whichfork);
void	__xfs_bmap_add_free(struct xfs_mount *mp, struct xfs_defer_ops *dfops,
			  xfs_fsblock_t bno, xfs_filblks_t len,
			  struct xfs_owner_info *oinfo, bool skip_discard);
void	xfs_bmap_compute_maxlevels(struct xfs_mount *mp, int whichfork);
int	xfs_bmap_first_unused(struct xfs_trans *tp, struct xfs_inode *ip,
		xfs_extlen_t len, xfs_fileoff_t *unused, int whichfork);
int	xfs_bmap_last_before(struct xfs_trans *tp, struct xfs_inode *ip,
		xfs_fileoff_t *last_block, int whichfork);
int	xfs_bmap_last_offset(struct xfs_inode *ip, xfs_fileoff_t *unused,
		int whichfork);
int	xfs_bmap_one_block(struct xfs_inode *ip, int whichfork);
int	xfs_bmapi_read(struct xfs_inode *ip, xfs_fileoff_t bno,
		xfs_filblks_t len, struct xfs_bmbt_irec *mval,
		int *nmap, int flags);
int	xfs_bmapi_write(struct xfs_trans *tp, struct xfs_inode *ip,
		xfs_fileoff_t bno, xfs_filblks_t len, int flags,
		xfs_fsblock_t *firstblock, xfs_extlen_t total,
		struct xfs_bmbt_irec *mval, int *nmap,
		struct xfs_defer_ops *dfops);
int	__xfs_bunmapi(struct xfs_trans *tp, struct xfs_inode *ip,
		xfs_fileoff_t bno, xfs_filblks_t *rlen, int flags,
		xfs_extnum_t nexts, xfs_fsblock_t *firstblock,
		struct xfs_defer_ops *dfops);
int	xfs_bunmapi(struct xfs_trans *tp, struct xfs_inode *ip,
		xfs_fileoff_t bno, xfs_filblks_t len, int flags,
		xfs_extnum_t nexts, xfs_fsblock_t *firstblock,
		struct xfs_defer_ops *dfops, int *done);
int	xfs_bmap_del_extent_delay(struct xfs_inode *ip, int whichfork,
		struct xfs_iext_cursor *cur, struct xfs_bmbt_irec *got,
		struct xfs_bmbt_irec *del);
void	xfs_bmap_del_extent_cow(struct xfs_inode *ip,
		struct xfs_iext_cursor *cur, struct xfs_bmbt_irec *got,
		struct xfs_bmbt_irec *del);
uint	xfs_default_attroffset(struct xfs_inode *ip);
int	xfs_bmap_collapse_extents(struct xfs_trans *tp, struct xfs_inode *ip,
		xfs_fileoff_t *next_fsb, xfs_fileoff_t offset_shift_fsb,
		bool *done, xfs_fsblock_t *firstblock,
		struct xfs_defer_ops *dfops);
int	xfs_bmap_insert_extents(struct xfs_trans *tp, struct xfs_inode *ip,
		xfs_fileoff_t *next_fsb, xfs_fileoff_t offset_shift_fsb,
		bool *done, xfs_fileoff_t stop_fsb, xfs_fsblock_t *firstblock,
		struct xfs_defer_ops *dfops);
int	xfs_bmap_split_extent(struct xfs_inode *ip, xfs_fileoff_t split_offset);
int	xfs_bmapi_reserve_delalloc(struct xfs_inode *ip, int whichfork,
		xfs_fileoff_t off, xfs_filblks_t len, xfs_filblks_t prealloc,
		struct xfs_bmbt_irec *got, struct xfs_iext_cursor *cur,
		int eof);

static inline void
xfs_bmap_add_free(
	struct xfs_mount		*mp,
	struct xfs_defer_ops		*dfops,
	xfs_fsblock_t			bno,
	xfs_filblks_t			len,
	struct xfs_owner_info		*oinfo)
{
	__xfs_bmap_add_free(mp, dfops, bno, len, oinfo, false);
}

enum xfs_bmap_intent_type {
	XFS_BMAP_MAP = 1,
	XFS_BMAP_UNMAP,
};

struct xfs_bmap_intent {
	struct list_head			bi_list;
	enum xfs_bmap_intent_type		bi_type;
	struct xfs_inode			*bi_owner;
	int					bi_whichfork;
	struct xfs_bmbt_irec			bi_bmap;
};

int	xfs_bmap_finish_one(struct xfs_trans *tp, struct xfs_defer_ops *dfops,
		struct xfs_inode *ip, enum xfs_bmap_intent_type type,
		int whichfork, xfs_fileoff_t startoff, xfs_fsblock_t startblock,
		xfs_filblks_t *blockcount, xfs_exntst_t state);
int	xfs_bmap_map_extent(struct xfs_mount *mp, struct xfs_defer_ops *dfops,
		struct xfs_inode *ip, struct xfs_bmbt_irec *imap);
int	xfs_bmap_unmap_extent(struct xfs_mount *mp, struct xfs_defer_ops *dfops,
		struct xfs_inode *ip, struct xfs_bmbt_irec *imap);

static inline int xfs_bmap_fork_to_state(int whichfork)
{
	switch (whichfork) {
	case XFS_ATTR_FORK:
		return BMAP_ATTRFORK;
	case XFS_COW_FORK:
		return BMAP_COWFORK;
	default:
		return 0;
	}
}

xfs_failaddr_t xfs_bmap_validate_extent(struct xfs_inode *ip, int whichfork,
		struct xfs_bmbt_irec *irec);

int	xfs_bmapi_remap(struct xfs_trans *tp, struct xfs_inode *ip,
		xfs_fileoff_t bno, xfs_filblks_t len, xfs_fsblock_t startblock,
		struct xfs_defer_ops *dfops, int flags);

#endif	/* __XFS_BMAP_H__ */
