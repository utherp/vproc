typedef struct vproc_optval_s {
    int number;
    enum { 
        boolean=1,
        integer,
        precision,
        string
    } type;
    union {
        unsigned char boolean;
        int integer;
        double precision;
        char *string;
    } of;
} vproc_optval;


typedef struct vproc_opts_s {
    const char *(*enum_opts)(void *priv, int opt_number);
    vproc_optval (*get_opt)(void *priv, int opt_number);
    int (*set_opt)(void *priv, vproc_optval *value);
} vproc_opts;


typedef struct vproc_hooks_s {
    int (*init)(void **priv);
    int (*clean)(void **priv);
    int (*start)(void *priv);
    int (*stop)(void *priv);
    int (*trigger)(void *priv, int trigger_number, int (*next_trigger)(void *priv, int next_trigger_number));
} vproc_hooks;


typedef struct vproc_instance_s {
    int type;
    const char *name;
    vproc_opts opts;
    vproc_hooks call;
    void *priv;
} vproc_instance;

//  inst.opts.enum_opts, inst.call.init, inst.priv (private data)


