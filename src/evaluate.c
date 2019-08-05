#include "private/common.h"
#include "jansson.h"
#include "private/jsonpath_ast.h"
#include "janssonpath_evaluate.h"
#include <string.h>

static jsonpath_function get_function(jsonpath_function_table_t* function_table, const char* name){
	size_t i;
	if (function_table) for (i = 0; i < function_table->size; ++i)
		if (!strcmp(function_table->names[i], name)) return function_table->functions[i];
	return NULL;
}

static const jsonpath_result_t error_result = { NULL,false,false,false };

static jsonpath_result_t jsonpath_evaluate_impl_basic(json_t* root, jsonpath_result_t curr_element, jsonpath_t* jsonpath, jsonpath_function_table_t* function_table, jsonpath_error_t* error);
#ifdef JANSSONPATH_CONSTANT_FOLD
static void jsonpath_constant_fold(jsonpath_t* jsonpath, jsonpath_result_t value) {
	assert (jsonpath);
	switch (jsonpath->tag) {
	case JSON_SINGLE:
		single_release(jsonpath->single);
		break;
	case JSON_INDEX:
		indexes_release(jsonpath->indexes);
		break;
	case JSON_UNARY:
		unary_release(jsonpath->unary);
		break;
	case JSON_BINARY:
		binary_release(jsonpath->binary);
		break;
	case JSON_ARBITRAY:
		arbitray_release(jsonpath->arbitrary);
		break; 
	case JSON_CONSTANT:
		jsonpath_decref(jsonpath->constant_result);
		break;
	default:
		break;
	}
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

static jsonpath_result_t jsonpath_evaluate_impl_path(json_t* root, jsonpath_result_t curr_element, path_indexes_t jsonpath, jsonpath_function_table_t* function_table, jsonpath_error_t* error){
	return error_result;
}

static jsonpath_result_t jsonpath_evaluate_impl_binary(json_t* root, jsonpath_result_t curr_element, path_indexes_t jsonpath, jsonpath_function_table_t* function_table, jsonpath_error_t* error) {
	return error_result;
}

static jsonpath_result_t jsonpath_evaluate_impl_arbitrary(json_t* root, jsonpath_result_t curr_element, path_indexes_t jsonpath, jsonpath_function_table_t* function_table, jsonpath_error_t* error) {
	return error_result;
}

static json_t* json_not(json_t* in){
	if (json_is_boolean(in)) return json_boolean(!json_boolean_value(in));
	else if (json_is_number(in)) return json_boolean(!json_number_value(in)); // should we allow number to be negatived?
	else return json_null();
} 

static json_t* json_neg(json_t* in) {
	if (json_is_boolean(in)) return json_boolean(!json_boolean_value(in));
	else if (json_is_number(in)) return json_boolean(!json_number_value(in)); // should we allow number to be negatived?
	else return json_null();
}

static json_t* json_bitnot(json_t* in) {
	if (json_is_boolean(in)) return json_boolean(!json_boolean_value(in));
	else if (json_is_number(in)) return json_boolean(!json_number_value(in)); // should we allow number to be negatived?
	else return json_null();
}

static jsonpath_result_t unary_deal_with_collection(
	jsonpath_result_t origin_result, json_t* (*oprator)(json_t*)
){
	if (!origin_result.is_collection) {
		return make_result(oprator(origin_result.value), false, true, origin_result.is_constant);
	}else{
		size_t index; json_t* value;
		jsonpath_result_t ret = make_result(json_array(), true, true, origin_result.is_constant);
		json_array_foreach(origin_result.value,index,value){
			json_t* mapped_element = oprator(value);
			json_array_append_new(ret.value, mapped_element);
		}
		return ret;
	}
}

static jsonpath_result_t evaluate_unary(path_unary_tag_t op, jsonpath_result_t oprand){
	jsonpath_result_t ret = error_result;
	switch(op){
	case UNARY_NOT:
		return unary_deal_with_collection(oprand, json_not);
	case UNARY_POS:
		return jsonpath_incref(oprand);
	case UNARY_NEG:
		return unary_deal_with_collection(oprand, json_neg);
	case UNARY_TO_LIST:
		if (oprand.is_collection) ret =  make_result(json_incref(oprand.value), false, true, oprand.is_constant);
		break;
	case UNARY_FROM_LIST:
		if (!oprand.is_collection) ret = make_result(json_incref(oprand.value), true, oprand.is_right_value, oprand.is_constant);
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
		return evaluate_unary(jsonpath->unary.tag, node);
	}
	case JSON_BINARY:
		return jsonpath_evaluate_impl_binary(root, curr_element, jsonpath->indexes, function_table, error);
	case JSON_ARBITRAY:
		return jsonpath_evaluate_impl_arbitrary(root, curr_element, jsonpath->indexes, function_table, error);
	default: break;
	}
	*error = jsonpath_error_unknown;
	return error_result;
}

JANSSONPATH_EXPORT jsonpath_result_t jsonpath_evaluate(json_t* root, jsonpath_t* jsonpath, jsonpath_function_table_t* function_table, jsonpath_error_t* error){
	json_t* dup_root = json_incref(root);
	jsonpath_result_t root_curr = make_result(dup_root, false, true, false);
	jsonpath_result_t ret = jsonpath_evaluate_impl(root, root_curr, jsonpath, function_table, error);
	json_decref(dup_root);
	return ret;
}
