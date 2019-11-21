
#define vproc_conversion struct _vproc_conversion


vproc_conversion {
    vproc_format *input_format;
    vproc_format *output_format;
    vproc_filter *filters[];
}

/************************************************
 * vproc_conversion:
 *   A conversion is a pair of input / output
 *   format pointers and a list of pointers to
 *   filters which support this conversion
 */
 
// vproc_filter ?

vproc_transcoder {
    vproc_format *input_format;
    vproc_format *output_format;
    vproc_conversion *conversions[];
}

/********************************************************
 * vproc_transcoder: 
 *   The transcoder structure contains the initial input
 *   format and final output format between a series of
 *   conversions.  The conversions are listed as an ordered
 *   array of vproc_conversion pointers.
 */

#define vproc_plugin_type enum _vproc_plugin_type
#define vproc_plugin_instance struct _vproc_plugin_instance
#define vproc_plugin struct _vproc_plugin

_vproc_plugin_type {
    Format_Plugin=1,
    Client_Plugin,
    Transcoder_Plugin
};


vproc_plugin_instance {
    vproc_plugin_instance *next;
    vproc_plugin *plugin;
    uint32  id;
    void *priv;
}

// Flags
#define VP_MM_SRC_OUTPUT (1<<0)   /* source output is possible */
#define VP_MM_SRC_DUP    (1<<1)   /* source is duplicated to output before processing */
#define VP_FM_CONVERSION (1<<2)   /* format conversion is being done */

vproc_module {
    char *name;
    void *handle;
    vproc_plugin *plugins[];
}

vproc_plugin {
    uint32 id;
    vproc_module *module;
    vproc_plugin_type type;
    vproc_plugin_instance *instances;
    _uint32 flags;      /* VP_FM_CONVERSION differentiates plugin as a filter or converter */
    int32 (*init) (vproc_plugin_instance *instance); //void **priv);
    int32 (*destroy) (vproc_plugin_instance *instance); //void **priv);
    int32 (*set_param) (vproc_plugin_instance *instance, uint32 param_id, void *value);
}

vproc_format_plugin {
    vproc_plugin general;
    vproc_conversion *conversions; 
    _int32 (*process_frame) (vproc_plugin_instance *instance, vproc_frame *source, vproc_frame *dest);
}

vproc_transcoder_plugin {
    vproc_plugin_general;

