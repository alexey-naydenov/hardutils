$# imports
$py(
import functools
)\
$# constants that contain actual template text
$py(
_STRUCT_DECL = 'struct {ns}{class}'
_NEW_DECL = '{rc_type} {ns}{class}_new(struct {ns}{class} **obj_pointer)'
_REF_DECL = 'struct {ns}{class} *{ns}{class}_ref(struct {ns}{class} *obj)'
_UNREF_DECL = 'struct {ns}{class} *{ns}{class}_unref(struct {ns}{class} *obj)'
_FREE_DECL = 'void {ns}{class}_free(struct {ns}{class} *obj)'
_HEADER_DECLS = ';\n'.join([_STRUCT_DECL, _NEW_DECL, _REF_DECL,
                            _UNREF_DECL, _FREE_DECL]) + ';'
_STRUCT_BEGIN_DEFN = _STRUCT_DECL + str(' {{\n'
                                        '\t{cnt_type} refcounter;')
_NEW_START_DEFN = _NEW_DECL + '\n'.join(
        [' {{',
         '\tstruct {ns}{class} *obj;',
         '\tobj = calloc(1, sizeof(struct {ns}{class}));',
         '\tif (!obj)',
         '\t\treturn -1;',
         '\tobj->refcount = 1;',])
_NEW_END_DEFN = '\n'.join(
        ['\t*obj_pointer = obj;',
         '\treturn 0;',
         '}}'])
_REF_DEFN = _REF_DECL + '\n'.join(
        [' {{',
         '\tif (obj == NULL)',
         '\t\treturn NULL;',
         '\tobj->refcount++;',
         '\treturn obj;',
         '}}'])
_UNREF_DEFN = _UNREF_DECL + '\n'.join(
        [' {{',
         '\tif (obj == NULL)',
         '\t\treturn NULL;',
         '\tobj->refcount--;',
         '\tif (obj->refcount > 0)',
         '\t\treturn obj;',
         '\t{ns}{class}_free(obj);',
         '\treturn NULL;',
         '}}'])
_FREE_START_DEFN = _FREE_DECL + '\n'.join(
        [' {{',
         '\tif (obj == NULL)',
         '\t\treturn;'])
_FREE_END_DEFN = '\n'.join(['\tfree(obj);', '}}'])
)\
$# helper functions
$py(
def args_to_dict(f):
    @functools.wraps(f)
    def wrapper(class_name, ns=None, rc_type=None, cnt_type=None):
        rc_type = rc_type if rc_type else return_code_type
        cnt_type = cnt_type if cnt_type else counter_type
        if ns:
            ns = ns + '_'
        else:
            if current_namespace:
                ns = current_namespace + '_'
            else:
                ns = ''
        template_dict = {'class': class_name, 'rc_type': rc_type, 'ns': ns,
                         'cnt_type': cnt_type}
        return f(template_dict)
    return wrapper
    )\
$# module level setters
$py(
counter_type = 'int'
def set_counter_type(counter_type_name):
    global counter_type
    counter_type = counter_type_name
    return ''
return_code_type = 'int'
def set_return_code_type(rc_type_name):
    global return_code_type
    return_code_type = rc_type_name
    return ''
current_namespace = ''
def set_namespace(ns):
    global current_namespace
    current_namespace = ns
    return ''
    )\
$extend(set_counter_type, set_return_code_type, set_namespace)\
$# header generation functions
$py(
@args_to_dict
def declare_object(template_dict):
    return _HEADER_DECLS.format(**template_dict)
    )\
$extend(declare_object)\
$# source generation functions
$py(
@args_to_dict
def start_struct(template_dict):
    return _STRUCT_BEGIN_DEFN.format(**template_dict)
@args_to_dict
def end_struct(template_dict):
    return '};\n'
@args_to_dict
def define_ref(template_dict):
    return _REF_DEFN.format(**template_dict)
@args_to_dict
def define_unref(template_dict):
    return _UNREF_DEFN.format(**template_dict)
@args_to_dict
def start_define_new(template_dict):
    return _NEW_START_DEFN.format(**template_dict)
@args_to_dict
def end_define_new(template_dict):
    return _NEW_END_DEFN.format(**template_dict)
@args_to_dict
def start_define_free(template_dict):
    return _FREE_START_DEFN.format(**template_dict)
@args_to_dict
def end_define_free(template_dict):
    return _FREE_END_DEFN.format(**template_dict)

    )\
$extend(start_struct, end_struct, start_define_new, end_define_new, 
        define_ref, define_unref, start_define_free, end_define_free)\
