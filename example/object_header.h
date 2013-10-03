$include("c_object.inc")\
$set_counter_type("int_fast8_t")\
$set_return_code_type("int_fast8_t")\
$set_namespace("ow")\
$declare_object("bus")

$start_struct("bus")
	int var;
	uint8_t *data;
$end_struct("bus")

$start_define_new("bus")
	obj->var = 10;
	obj->data = calloc(obj->var, sizeof(uint8_t));
$end_define_new("bus")

$define_ref("bus")

$define_unref("bus")

$start_define_free("bus")
	free(obj->data);
$end_define_free("bus")
