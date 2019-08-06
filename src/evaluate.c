#include "jansson.h"
#include "private/common.h"
#include "private/jansson_memory.h"
#include "private/jsonpath_ast.h"
#include "janssonpath_evaluate.h"
#include <string.h>

#ifdef JANSSONPATH_SUPPORT_REGEX
#include "private/regex_impl.h"
#endif

static const jsonpath_error_t jsonpath_error_collection_oprand = {
	true, 0xaa, "Collection used as oprand in incompatible opration", NULL
};

static jsonpath_function get_function(jsonpath_function_table_t* function_table, const char* name){
	size_t i;
	if (function_table) for (i = 0; i < function_table->size; ++i)
		if (!strcmp(function_table->names[i], name)) return function_table->functions[i];
	return NULL;
}

static const jsonpath_result_t error_result = { NULL,false,false,false };

static jsonpath_result_t jsonpath_evaluate_impl_basic(json_t* root, jsonpath_result_t curr_element, jsonpath_t* jsonpath, jsonpath_function_table_t* function_table, jsonpath_error_t* error);
#ifdef JANSSONPATH_CONSTANT_FOLD
void JANSSONPATH_NO_EXPORT jsonpath_release_no_free(jsonpath_t* jsonpath);
static void jsonpath_constant_fold(jsonpath_t* jsonpath, jsonpath_result_t value) {
	assert (jsonpath);
	jsonpath_release_no_free(jsonpath);
	jsonpath->tag = JSON_CONSTANT;
	jsonpath->constant_result = value;
}

static jsonpath_result_t jsonpath_evaluate_impl_constant_fold(json_t* root, jsonpath_result_t curr_element, jsonpath_t* jsonpath, jsonpath_function_table_t* function_table, jsonpath_error_t* error) {
	if(jsonpath->tag == JSON_CONSTANT){
		return jsonpath->constant_result;
	}
	jsonpath_result_t ret = jsonpath_evaluate_impl_basic(root, curr_element, jsonpath, function_table, error);
	if (!error->abort && ret.is_constant) {
		jsonpath_constant_fold(jsonpath, jsonpath_incref(ret));
	}
	return ret;
}
#define jsonpath_evaluate_impl jsonpath_evaluate_impl_constant_fold
#else
#define jsonpath_evaluate_impl jsonpath_evaluate_impl_basic
#endif

static jsonpath_result_t make_result(json_t *value, bool is_collection, bool is_right_value, bool is_constant){
	jsonpath_result_t ret = { value,is_collection,is_right_value,is_constant };
	return ret;
}

static json_t* json_object_to_array(json_t *object){
	assert(json_is_object(object));
	const char* key; json_t* value;
	json_t* ret=json_array();
	json_object_foreach(object, key, value){
		json_array_append(ret, value);
	}
	return ret;
}

static jsonpath_result_t path_deal_with_collection(
	json_t* root, jsonpath_result_t curr_element,
	path_index_t operator_, jsonpath_result_t node, jsonpath_function_table_t* function_table, jsonpath_error_t* error
);

static json_t* json_get_all_property(json_t* node){
	if (json_is_array(node)) return json_incref(node);
	else if (json_is_object(node)) return json_object_to_array(node);
	else return json_array();
}

static size_t json_array_index_translate(json_int_t index, size_t array_size){
	if (index < 0) index = array_size + index; // for -n
	size_t ret = (index < 0) ? 0 : index;
	if (ret >= array_size) ret = array_size - 1;
	return ret;
}

static jsonpath_result_t jsonpath_evaluate_impl_simple_index(jsonpath_result_t node, json_t* simple_index){
	assert(!node.is_collection);
	if (simple_index) {
		return make_result(json_get_all_property(node.value), true, true, node.is_constant);
	}
	else if (json_is_string(simple_index)) {
		return make_result(json_object_get(node.value, json_string_value(simple_index)), false, true, node.is_constant);
	}
	else if (json_is_number(simple_index)) {
		if (json_is_array(node.value)) return error_result;
		json_int_t index = json_is_integer(simple_index) ? json_integer_value(simple_index) : (json_int_t)json_real_value(simple_index);
		size_t array_size = json_array_size(node.value);
		return make_result(json_array_get(node.value, json_array_index_translate(index, array_size)), false, true, node.is_constant);
	}
	else if (json_is_null(simple_index)) {
		size_t size;
		if (json_is_array(node.value)) {
			size = json_array_size(node.value);
		}
		else if (json_is_array(node.value)) {
			size = json_object_size(node.value);
		}
		else {
			size = 0;
		}
		return make_result(json_integer(size), false, true, node.is_constant);
	}else{
		return error_result;
	}
}

static jsonpath_result_t jsonpath_evaluate_impl_path_simple(json_t* root, jsonpath_result_t curr_element, jsonpath_result_t node, path_index_t jsonpath, jsonpath_function_table_t* function_table, jsonpath_error_t* error) {
	assert(!node.is_collection);
	switch (jsonpath.tag) {
	case INDEX_SUB_SIMPLE:
		return jsonpath_evaluate_impl_simple_index(node, jsonpath.simple_index);
	case INDEX_DOT_RECURSIVE: {
		json_t* layer = json_get_all_property(node.value);
		json_t* all_node = json_incref(layer);
		while(json_array_size(layer)){
			json_t* new_layer = json_array();
			size_t index; json_t* value;
			json_array_foreach(layer, index, value) {
				json_t* get = json_get_all_property(value);
				json_array_extend(new_layer, get);
				json_decref(get);
			}
			json_decref(layer);
			layer = new_layer;
			json_array_extend(all_node, layer);
		}
		json_decref(layer);
		jsonpath_result_t all_node_result = { all_node, true, true, node.is_constant };

		path_index_t fake_path_index = jsonpath; // no need to release
		fake_path_index.tag = INDEX_SUB_SIMPLE;
		jsonpath_result_t ret = path_deal_with_collection(root, curr_element, fake_path_index, all_node_result, function_table, error);
		jsonpath_decref(all_node_result);
		return ret;
	}
	case INDEX_SUB_EXP: {
		jsonpath_result_t sub_exp_result = jsonpath_evaluate_impl(root, curr_element, jsonpath.expression, function_table, error);
		if (error->abort) return error_result;
		jsonpath_result_t ret =jsonpath_evaluate_impl_simple_index(node, sub_exp_result.value);
		jsonpath_decref(sub_exp_result);
		return ret;
	}
	case INDEX_SUB_RANGE:{
		if (!json_is_array(node.value)) return error_result;

		jsonpath_result_t ret = error_result;
		jsonpath_result_t range_json[2] = { { NULL,false,true,true }, { NULL,false,true,true } };
		if (jsonpath.range[0]) {
			range_json[0] = jsonpath_evaluate_impl(root, curr_element, jsonpath.range[0], function_table, error);
			if (error->abort) return error_result;
			if (range_json[0].is_collection) {
				*error = jsonpath_error_collection_oprand;
				goto range0;
			}
		}
		if (jsonpath.range[1]) {
			range_json[1] = jsonpath_evaluate_impl(root, curr_element, jsonpath.range[1], function_table, error);
			if (error->abort) goto range0;
			if (range_json[1].is_collection) {
				*error = jsonpath_error_collection_oprand;
				goto range1;
			}
		}

		size_t array_size = json_array_size(node.value);
		json_int_t index[2] = { 0, array_size };
		if(range_json[0].value){
			if (!json_is_number(range_json[0].value)) goto range1;
			index[0] = json_is_integer(range_json[0].value) ? json_integer_value(range_json[0].value) : (json_int_t)json_real_value(range_json[0].value);
		}
		if (range_json[1].value) {
			if (!json_is_number(range_json[1].value)) goto range1;
			index[1] = json_is_integer(range_json[1].value) ? json_integer_value(range_json[1].value) : (json_int_t)json_real_value(range_json[1].value);
		}
		
		size_t begin = json_array_index_translate(index[0], array_size), end = json_array_index_translate(index[1], array_size);
		
		ret = make_result(json_array(), true, true, node.is_constant && range_json[0].is_constant && range_json[1].is_constant);
		size_t i;
		for(i=begin;i<end;++i){
			json_array_append(ret.value, json_array_get(node.value, i));
		}

	range1:
		jsonpath_decref(range_json[1]);
	range0:
		jsonpath_decref(range_json[0]);
		return ret;
	}
	case INDEX_FILTER: {

#define for_body {\
		jsonpath_result_t curr = make_result(value, false, false, false);\
		jsonpath_result_t cond = jsonpath_evaluate_impl(root, curr, jsonpath.expression, function_table, error);\
		if (error->abort)goto fail;\
		if (cond.is_collection) {\
			jsonpath_decref(curr);\
			*error = jsonpath_error_collection_oprand;\
			goto fail;\
		}\
		if (json_is_true(curr.value))json_array_append(ret.value, value);\
		jsonpath_decref(curr);\
		}

		jsonpath_result_t ret = make_result(json_array(), true, true, node.is_constant);
		if(json_is_object(node.value)){
			const char* key; json_t* value;
			json_object_foreach(node.value, key, value) for_body
			return ret;
		}else if (json_is_object(node.value)) {
			size_t index; json_t* value;
			json_array_foreach(node.value, index, value) for_body
			return ret;
		}else{
			return ret;
		}
#undef for_body
	fail:
		jsonpath_decref(ret);
		return error_result;
	}
	default:
		return error_result;
	}
}

static jsonpath_result_t path_deal_with_collection(
	json_t* root, jsonpath_result_t curr_element,
	path_index_t operator_, jsonpath_result_t node, jsonpath_function_table_t* function_table, jsonpath_error_t* error
) {
	if (!node.is_collection) {
		return jsonpath_evaluate_impl_path_simple(root, curr_element, node, operator_, function_table, error);
	}
	else {
		size_t index; json_t* value;
		jsonpath_result_t ret = make_result(json_array(), true, true, node.is_constant);
		json_array_foreach(node.value, index, value) {
			jsonpath_result_t mapped_element = jsonpath_evaluate_impl_path_simple(root, curr_element, make_result(value,false,node.is_right_value,node.is_constant), operator_, function_table, error);
			if (error->abort) {
				jsonpath_decref(ret);
				return error_result;
			}
			if (mapped_element.is_constant) ret.is_constant = false;
			if(!mapped_element.is_collection){
				json_array_append_new(ret.value, mapped_element.value);
			}else{
				json_array_extend(ret.value, mapped_element.value);
				jsonpath_decref(mapped_element);
			}
		}
		return ret;
	}
}

// note that root is absolute, thus second $ in (*$.a[1:20])[$.index] does not refer to (*$.a[1:20])
static jsonpath_result_t jsonpath_evaluate_impl_path(json_t* root, jsonpath_result_t curr_element, path_indexes_t jsonpath, jsonpath_function_table_t* function_table, jsonpath_error_t* error){
	// inner expression will take curr_root as their curr_element
	jsonpath_result_t ret = jsonpath_evaluate_impl(root, curr_element, jsonpath.root_node, function_table, error);
	if (error->abort) return error_result;

	size_t i;
	for (i = 0; i < jsonpath.size; ++i) {
		jsonpath_result_t new_ret = path_deal_with_collection(root, ret, jsonpath.indexes[i], ret, function_table, error);
		jsonpath_decref(ret);
		if (error->abort) return error_result;
		ret = new_ret;
	}

	return ret;
}

static json_t* json_binary(path_binary_tag_t operator_, json_t* lhs, json_t* rhs, jsonpath_error_t* error) {
	// we don't abort at type error. instead we return json_null()
	switch(operator_){
	case BINARY_ADD: {
		if (!json_is_number(lhs) || !json_is_number(rhs)) return json_null();
		if (json_is_real(lhs) || json_is_real(rhs)) {
			return json_real(json_real_value(lhs) + json_real_value(rhs));
		}else {
			return json_integer(json_integer_value(lhs) + json_integer_value(rhs));
		}
	}
	case BINARY_MNU: {
		if (!json_is_number(lhs) || !json_is_number(rhs)) return json_null();
		if (json_is_real(lhs) || json_is_real(rhs)) {
			return json_real(json_real_value(lhs) - json_real_value(rhs));
		}else {
			return json_integer(json_integer_value(lhs) - json_integer_value(rhs));
		}
	}
	case BINARY_MUL: {
		if (!json_is_number(lhs) || !json_is_number(rhs)) return json_null();
		if (json_is_real(lhs) || json_is_real(rhs)) {
			return json_real(json_real_value(lhs) * json_real_value(rhs));
		}else {
			return json_integer(json_integer_value(lhs) * json_integer_value(rhs));
		}
	}
	case BINARY_DIV: {
		if (!json_is_number(lhs) || !json_is_number(rhs)) return json_null();
		if (json_is_real(lhs) || json_is_real(rhs)) {
			return json_real(json_number_value(lhs) / json_number_value(rhs));
		}else {
			return json_integer(json_integer_value(lhs) / json_integer_value(rhs));
		}
	}
	case BINARY_REMINDER: {
		if (json_is_integer(lhs) && json_is_integer(rhs)) {
			return json_integer(json_integer_value(lhs) % json_integer_value(rhs));
		}else {
			return json_null();
		}
	}
	case BINARY_BITAND: {
		if (json_is_integer(lhs) && json_is_integer(rhs)) {
			return json_integer(json_integer_value(lhs) & json_integer_value(rhs));
		}else if (json_is_boolean(lhs) && json_is_boolean(rhs)) {
			return json_boolean(json_boolean_value(lhs) && json_boolean_value(rhs));
		}else {
			return json_null();
		}
	}
	case BINARY_BITXOR: {
		if (json_is_integer(lhs) && json_is_integer(rhs)) {
			return json_integer(json_integer_value(lhs) ^ json_integer_value(rhs));
		}
		else if (json_is_boolean(lhs) && json_is_boolean(rhs)) {
			unsigned lhs_ = json_boolean_value(lhs) ? 1u : 0u;
			unsigned rhs_ = json_boolean_value(rhs) ? 1u : 0u;
			return json_boolean(lhs_^rhs_);
		}
		else {
			return json_null();
		}
	}
	case BINARY_BITOR: {
		if (json_is_integer(lhs) && json_is_integer(rhs)) {
			return json_integer(json_integer_value(lhs) | json_integer_value(rhs));
		}
		else if (json_is_boolean(lhs) && json_is_boolean(rhs)) {
			return json_boolean(json_boolean_value(lhs) || json_boolean_value(rhs));
		}
		else {
			return json_null();
		}
	}
	case BINARY_AND: {
		bool lhs_, rhs_; // silly MSVC can not figure out that they must be assigned
		if (json_is_number(lhs)) {
			lhs_ = json_number_value(lhs);
		}else if(json_is_boolean(lhs)){
			lhs_ = json_boolean_value(lhs);
		}else {
			return json_null();
		}
		if (json_is_number(rhs)) {
			rhs_ = json_number_value(rhs);
		}
		else if (json_is_boolean(rhs)) {
			rhs_ = json_boolean_value(rhs);
		}
		else {
			return json_null();
		}
		return json_boolean(lhs_ && rhs_);
	}
	case BINARY_OR: {
		bool lhs_, rhs_;
		if (json_is_number(lhs)) {
			lhs_ = json_number_value(lhs);
		}
		else if (json_is_boolean(lhs)) {
			lhs_ = json_boolean_value(lhs);
		}
		else {
			return json_null();
		}
		if (json_is_number(rhs)) {
			rhs_ = json_number_value(rhs);
		}
		else if (json_is_boolean(rhs)) {
			rhs_ = json_boolean_value(rhs);
		}
		else {
			return json_null();
		}
		return json_boolean(lhs_ || rhs_);
	}
	case BINARY_LSH: {
		if (json_is_integer(lhs) && json_is_integer(rhs)) {
			return json_integer(json_integer_value(lhs) << json_integer_value(rhs));
		}else {
			return json_null();
		}
	}
	case BINARY_RSH: {
		if (json_is_integer(lhs) && json_is_integer(rhs)) {
			return json_integer(json_integer_value(lhs) >> json_integer_value(rhs));
		}
		else {
			return json_null();
		}
	}
	case BINARY_EQ: {
		return json_boolean(json_equal(lhs, rhs));
	}
	case BINARY_NE: {
		return json_boolean(!json_equal(lhs, rhs));
	}
	case BINARY_LT: {
		if (!json_is_number(lhs) || !json_is_number(rhs)) return json_null();
		if (json_is_real(lhs) || json_is_real(rhs)) {
			return json_boolean(json_real_value(lhs) < json_real_value(rhs));
		}
		else {
			return json_boolean(json_integer_value(lhs) < json_integer_value(rhs));
		}
	}
	case BINARY_GT: {
		if (!json_is_number(lhs) || !json_is_number(rhs)) return json_null();
		if (json_is_real(lhs) || json_is_real(rhs)) {
			return json_boolean(json_real_value(lhs) > json_real_value(rhs));
		}
		else {
			return json_boolean(json_integer_value(lhs) > json_integer_value(rhs));
		}
	}
	case BINARY_LE: {
		if (!json_is_number(lhs) || !json_is_number(rhs)) return json_null();
		if (json_is_real(lhs) || json_is_real(rhs)) {
			return json_boolean(json_real_value(lhs) <= json_real_value(rhs));
		}
		else {
			return json_boolean(json_integer_value(lhs) <= json_integer_value(rhs));
		}
	}
	case BINARY_GE: {
		if (!json_is_number(lhs) || !json_is_number(rhs)) return json_null();
		if (json_is_real(lhs) || json_is_real(rhs)) {
			return json_boolean(json_real_value(lhs) >= json_real_value(rhs));
		}
		else {
			return json_boolean(json_integer_value(lhs) >= json_integer_value(rhs));
		}
	}
	case BINARY_ARRAY_CON: {
		if (!json_is_array(lhs) || !json_is_array(rhs)) return json_null();
		json_t* ret = json_copy(lhs);
		if(!json_array_extend(ret, rhs)) return json_null();
		return ret;
	}
#ifdef JANSSONPATH_SUPPORT_REGEX
	case BINARY_REGEX: {
		if (!json_is_string(lhs) || !json_is_string(rhs)) return json_null();
		jansson_regex_t* regex = regex_compile(json_string_value(rhs),error);
		if(error->abort) return json_null();
		json_t* ret = json_boolean(regex_match(json_string_value(lhs), regex)); // TODO: cache regex for constant
		regex_free(regex);
		return ret;
	}
#endif
	default:
		assert(false);
		return json_null();
	}
}


static jsonpath_result_t binary_deal_with_collection(
	path_binary_tag_t operator_, jsonpath_result_t lhs, jsonpath_result_t rhs, jsonpath_error_t* error
) {
	if (!lhs.is_collection) {
		return make_result(json_binary(operator_, lhs.value, rhs.value, error), false, true, lhs.is_constant && rhs.is_constant);
	}
	else {
		size_t index; json_t* value;
		jsonpath_result_t ret = make_result(json_array(), true, true, lhs.is_constant && rhs.is_constant);
		json_array_foreach(lhs.value, index, value) {
			json_t* mapped_element = json_binary(operator_, value, rhs.value, error);
			json_array_append_new(ret.value, mapped_element);
		}
		return ret;
	}
}

// we don't accept right oprand to be collection
// to do something like 1-$.*, you can translate it into -$.*+1
static jsonpath_result_t jsonpath_evaluate_impl_binary(json_t* root, jsonpath_result_t curr_element, path_binary_t jsonpath, jsonpath_function_table_t* function_table, jsonpath_error_t* error) {
	jsonpath_result_t ret = error_result;
	
	jsonpath_result_t lhs_result = jsonpath_evaluate_impl(root, curr_element, jsonpath.lhs, function_table, error);
	if (error->abort) goto lhs_release;
	jsonpath_result_t rhs_result = jsonpath_evaluate_impl(root, curr_element, jsonpath.rhs, function_table, error);
	if (error->abort) {
		goto lhs_release;
	}
	if(rhs_result.is_collection){
		*error = jsonpath_error_collection_oprand;
		goto rhs_release;
	}

	ret = binary_deal_with_collection(jsonpath.tag, lhs_result, rhs_result, error);
rhs_release:
	jsonpath_decref(rhs_result);
lhs_release:
	jsonpath_decref(lhs_result);
	return ret;
}

// we simply forbid to call function against collection
// if you want to deal with collection, you can cast it into json_array with to_array
static jsonpath_result_t jsonpath_evaluate_impl_arbitrary(json_t* root, jsonpath_result_t curr_element, path_arbitrary_t jsonpath, jsonpath_function_table_t* function_table, jsonpath_error_t* error) {
	assert(json_is_string(jsonpath.func_name));
	jsonpath_function func = get_function(function_table, json_string_value(jsonpath.func_name));
	if (!func) {
		error->abort = true;
		error->code = 0xa1;
		error->reason = "Function not found in the table";
		error->extra = (void*)jsonpath.func_name;
		return error_result;
	}

	jsonpath_result_t ret = error_result;
	size_t arg_n;
	json_t** args = do_malloc(jsonpath.size * sizeof(json_t*));

	// we don't assume functon call to be stateless and pure functional, so it's not constant even if all arguments are constant.
	for (arg_n = 0; arg_n < jsonpath.size; ++arg_n) {
		jsonpath_result_t arg = jsonpath_evaluate_impl(root, curr_element, jsonpath.nodes[arg_n], function_table, error);
		if (!error->abort && arg.is_collection) {
			*error = jsonpath_error_collection_oprand;
			jsonpath_decref(arg);
		}
		if (error->abort) goto args_release;
		args[arg_n] = arg.value;
	}
	ret = make_result(func(args, jsonpath.size), false, true, false);
	size_t i;
args_release: // simple dumb C have no label break, so even do{}while(0); does not work here
	for(i=0;i<arg_n;++i) json_decref(args[i]);
	do_free(args);
	return ret;
}

static json_t* json_not(json_t* in){
	if (json_is_boolean(in)) return json_boolean(!json_boolean_value(in));
	else if (json_is_number(in)) return json_boolean(!json_number_value(in)); // should we allow number to be negatived?
	else return json_null();
} 

static json_t* json_neg(json_t* in) {
	if (json_is_boolean(in)) {
		return json_boolean(!json_boolean_value(in));
	}else if (json_is_integer(in)) {
		return json_integer(-json_integer_value(in));
	}else if (json_is_real(in)) {
		return json_real(-json_real_value(in));
	}else {
		return json_null();
	}
}

static json_t* json_bitnot(json_t* in) {
	if (json_is_boolean(in)) return json_boolean(!json_boolean_value(in));
	else if (json_is_integer(in)) {
		return json_integer(~json_integer_value(in));
	}else {
		return json_null();
	}
}

static jsonpath_result_t unary_deal_with_collection(
	jsonpath_result_t origin_result, json_t* (*operator_)(json_t*)
){
	if (!origin_result.is_collection) {
		return make_result(operator_(origin_result.value), false, true, origin_result.is_constant);
	}else{
		size_t index; json_t* value;
		jsonpath_result_t ret = make_result(json_array(), true, true, origin_result.is_constant);
		json_array_foreach(origin_result.value,index,value){
			json_t* mapped_element = operator_(value);
			json_array_append_new(ret.value, mapped_element);
		}
		return ret;
	}
}

static jsonpath_result_t evaluate_unary(path_unary_tag_t op, jsonpath_result_t oprand, jsonpath_error_t* error){
	jsonpath_result_t ret = error_result;
	switch(op){
	case UNARY_NOT:
		return unary_deal_with_collection(oprand, json_not);
	case UNARY_POS:
		return jsonpath_incref(oprand);
	case UNARY_NEG:
		return unary_deal_with_collection(oprand, json_neg);
	case UNARY_TO_ARRAY:
		if (oprand.is_collection) ret =  make_result(json_incref(oprand.value), false, true, oprand.is_constant); // its content can be left value and it cannot be recovered by from_list
		error->abort = true;
		error->code = 0xa3;
		error->reason = "to_array(&) expecting collection oprand";
		error->extra = NULL;
		break;
	// should not apply to collection, as it breaks rule of *&x = x = &* x
	case UNARY_FROM_ARRAY:
		if (!oprand.is_collection && json_is_array(oprand.value)) ret = make_result(json_incref(oprand.value), true, oprand.is_right_value, oprand.is_constant);
		error->abort = true;
		error->code = 0xa4;
		error->reason = "from_array(*) expecting array(non-collection)";
		error->extra = NULL;
		break;
	case UNARY_BITNOT:
		return unary_deal_with_collection(oprand, json_bitnot);
	default: break;
	}
	return ret;
}

static jsonpath_result_t jsonpath_evaluate_impl_basic(json_t* root, jsonpath_result_t curr_element, jsonpath_t* jsonpath, jsonpath_function_table_t* function_table, jsonpath_error_t* error) {
	assert(!curr_element.is_collection);
	switch (jsonpath->tag) {
	case JSON_SINGLE:// single does not promote to collection
		switch (jsonpath->single.tag) {
		case SINGLE_ROOT:
			return make_result(json_incref(root), false, false, false);
		case SINGLE_CURR:
			return jsonpath_incref(curr_element);
		case SINGLE_CONST:
			return make_result(json_incref(jsonpath->single.constant), false, true, true);
		default: break;
		}
	case JSON_INDEX:
		return jsonpath_evaluate_impl_path(root, curr_element, jsonpath->indexes, function_table, error);
	case JSON_UNARY: {
		jsonpath_result_t node = jsonpath_evaluate_impl_basic(root, curr_element, jsonpath->unary.node, function_table, error);
		if (error->abort) return node;
		jsonpath_result_t ret = evaluate_unary(jsonpath->unary.tag, node, error);
		jsonpath_decref(node);
		return ret;
	}
	case JSON_BINARY:
		return jsonpath_evaluate_impl_binary(root, curr_element, jsonpath->binary, function_table, error);
	case JSON_ARBITRAY:
		return jsonpath_evaluate_impl_arbitrary(root, curr_element, jsonpath->arbitrary, function_table, error);
	default: break;
	}
	*error = jsonpath_error_unknown;
	return error_result;
}

JANSSONPATH_EXPORT jsonpath_result_t jsonpath_evaluate(json_t* root, jsonpath_t* jsonpath, jsonpath_function_table_t* function_table, jsonpath_error_t* error){
	*error = jsonpath_error_ok;
	json_t* dup_root = json_incref(root);
	jsonpath_result_t root_curr = make_result(dup_root, false, true, false);
	jsonpath_result_t ret = jsonpath_evaluate_impl(root, root_curr, jsonpath, function_table, error);
	json_decref(dup_root);
	return ret;
}