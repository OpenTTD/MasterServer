/* $Id$ */

#ifndef NEWGRF_CONFIG_H
#define NEWGRF_CONFIG_H

/* GRF config bit flags */
enum {
	GCF_DISABLED,
	GCF_NOT_FOUND,
	GCF_ACTIVATED,
	GCF_SYSTEM,
	GCF_UNSAFE,
	GCF_STATIC,
	GCF_COPY,      ///< The data is copied from a grf in _all_grfs
};

struct GRFIdentifier {
	/** We want to be able to make an uninitialized GRF */
	GRFIdentifier() {}

	/**
	 * Copy the important GRF data into the new GRF
	 * @param grf the grf to get the data from
	 */
	GRFIdentifier(const GRFIdentifier *grf)
	{
		this->grfid = grf->grfid;
		memcpy(this->md5sum, grf->md5sum, sizeof(this->md5sum));
	}

	uint32 grfid;     ///< The GRF ID of this GRF
	uint8 md5sum[16]; ///< The MD5 checksum of this GRF
};

/**
 * Element in a linked list of GRFConfigs, which is used to store
 * the GRF configuration of a game server.
 */
struct GRFConfig : public GRFIdentifier {
	uint8 flags;            ///< Flags (disabled states etc) of a GRF
	struct GRFConfig *next; ///< The next GRF in a configuration
};

/** Comparator for the GRFs */
struct GRFComparator
{
	/**
	 * Compare two GRFs on their GRF ID and MD5 checksum
	 */
	bool operator()(const GRFIdentifier* s1, const GRFIdentifier* s2) const
	{
		int ret;

		ret = s1->grfid - s2->grfid;
		if (ret == 0) ret = memcmp(s1->md5sum, s2->md5sum, sizeof(s1->md5sum));

		return ret < 0;
	}
};

#endif /* NEWGRF_CONFIG_H */
