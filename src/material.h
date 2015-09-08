#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#ifdef __cplusplus
extern "C" {
#endif

	struct material;
	int material_size(int pid);
	struct material *material_init(struct material *m, int pid);
	int material_uniform(struct material *m, int idx, int n, const float *v);
	int material_texture(struct material *m, int channel, int tid);
	void material_apply(struct material *m, int pid);

#ifdef __cplusplus
};
#endif
#endif // _MATERIAL_H_