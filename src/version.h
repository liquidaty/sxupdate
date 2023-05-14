#ifndef SXUPDATE_VERSION_H
#define SXUPDATE_VERSION_H

int sxupdate_version_cmp(struct sxupdate_semantic_version v1, struct sxupdate_semantic_version v2);

void sxupdate_version_free(struct sxupdate_version *v);

#endif
