# Janssonpath
Janssonpath 是 [Jansson](http://www.digip.org/jansson/)([Github](https://github.com/akheron/jansson)) 的 JSONPath 实现和扩展，这份文档描述的版本为 2.x。

Janssonpath 增加了一些常见的类似 C 语言的操作符，扩展了语法以使整体逻辑更加一致。

此外 Janssonpath 考虑到 JSONPath 作为一门语言的潜力，在特性和接口的设计上支持将 Janssonpath 作为其他语言的子语言使用。

## 构建、在项目中使用

Janssonpath 依赖 Jansson，如果想要支持正则表达式，还需要环境包含 regex.h 或 PCRE2。

获取源码后使用2.6以上版本的 CMake 构建。运行 configure 选择特性，如果 CMake 没能正确找到依赖的位置，为 CMake 指定依赖的 include 路径和链接库的路径，或者设置环境变量 JANSSON_DIR 和 PCRE2_DIR 后重新 configure。构建 INSTALL 目标会将动态库和静态库安装到指定目录，默认为合适的系统路径。

在项目中包含 include/janssonpath.h。 要使用动态库，确保 bin/janssonpath.so(.dll) 以及 Jansson、PCRE2（如果支持正则表达式）的动态库可被访问，在 Windows 下需要链接 lib/janssonpath.lib；要使用静态库，链接 lib/janssonpath_static.a(.lib)。

## 使用

类似 Jansson，用户需要创建一个`jsonpath_error_t`变量并将指针传入 API 以获取错误信息。

`jsonpath_error_t`的 code 成员不为0时为警告或错误。abort 成员为 true 表示出现了不可容忍的错误，此时不应当使用 API 的返回结果。 reason 成员为以'\0'结尾的字符串，用自然语言描述警告或错误的原因。

Janssonpath 采用先编译后求值的方式。

### 编译

```c++
jsonpath_t* jsonpath_compile_ranged(const char* jsonpath_begin, const char** pjsonpath_end, jsonpath_error_t* error);
jsonpath_t* jsonpath_compile(const char* jsonpath_begin, jsonpath_error_t* error);
jsonpath_t* jsonpath_parse(const char** p_jsonpath_begin, jsonpath_error_t* error);
```

编译的 API 返回编译结果，用户负责调用`jsonpath_release()`释放它们。当出现错误时返回 NULL。

`jsonpath_compile`接受以'\0'结尾的字符串。

`jsonpath_parse`接受`*p_jsonpath_begin`以'\0'结尾的字符串，容许在 Janssonpath 后有其他字符，贪婪地从给定字符串前缀中编译Janssonpath，并将`*p_jsonpath_begin`设置为 Janssonpath 结束的位置（超过1）。

`jsonpath_compile_ranged`将`jsonpath_begin`到`*pjsonpath_end`（不包含/超过1）作为范围编译 Janssonpath 表达式。`*pjsonpath_end`为NULL或`pjsonpath_end`为NULL的情形下以字符串的'\0'为结尾。`pjsonpath_end`不为NULL的情况下，像`jsonpath_parse` 一样接受前缀并在返回时设置`*pjsonpath_end`为 Janssonpath 结束的位置（超过1）。

### 求值

```c++
typedef struct jsonpath_result_t {
	json_t* value;
	bool is_collection : 1;
	bool is_right_value : 1;
	bool is_constant : 1;
}jsonpath_result_t;
jsonpath_result_t jsonpath_evaluate(json_t* root, jsonpath_t* jsonpath, jsonpath_symbol_lookup_t* symbols, jsonpath_error_t* error);
```

`jsonpath_evaluate`以 `root`参数传入 JSON 树的根节点。根节点可以为其他 JSON 树的子节点，可以是任意 JSON 类型（不限于 JSON object）。

Janssonpath 在风格上尽量与 Jansson 保持一致，返回的结果以新建引用的`json_t*`表示，用户负责最终将结果释放（减少引用计数）。可以调用`jsonpath_decref()`，其效果等同于对 value 成员调用`json_decref()`。

为了返回的节点可以被用于修改 JSON 树，也为了性能考虑，（如果返回的节点来自 JSON 树）返回的节点通常指向 JSON 树中的节点，修改返回节点会导致被指向的节点的改变，如果这不是你期望的效果，请在修改前按自己的需求进行浅/深复制。

Janssonpath 进行了一些扩展，使得一些原本只能在`[(expression)]` `[?(expression)]`中使用的语法可以作为最外层节点，甚至`1+1`也是合法的 Janssonpath，其结果是并不指向 JSON 树的值为2的 integer 节点。仅作为提示，Janssonpath 区分“左值”和”右值“（`is_right_value`），返回的节点是左值意味着它指向 JSON 树中的节点；但右值不一定不包含指向 JSON 树中的节点。

由于 JSONPath 中的一些操作会得到不止一个的结果，如 `$..price`，`$.book[1:24]` ，Janssonpath 的返回结果需要区分单个 JSON 节点与复数 JSON 节点。前者被称为单个节点，后者被称为集合（`is_collection`）。集合表示为 JSON 数组，其内容表示集合的成员。

在求值过程中 Janssonpath 可能能识别出表达式的一些部分是常量（` is_constant`），并且会在返回的结果中指出最终结果是否是常量。结果没有指出常量并不意味着一定不是常量，如`$.price-$.price`这样并不能简单识别的情况。如果构建时选择开启常量折叠，识别出的常量可能会加速之后的求值。

Janssonpath 支持自定义函数、变量，填充 `jsonpath_symbol_lookup_t` 结构并传入指针使用这项特性。不使用这项特性可以传入空指针。

### 过时接口

以下接口为旧版本 Jansson （1.X）的遗留，不建议使用。

```c++
typedef struct path_result {
	json_t* result;
	int is_collection;
} path_result;

json_t* json_path_get(json_t* json, const char* path);
path_result json_path_get_distinct(json_t* json, const char* path);
```

传入根节点与以字符串表示的 Janssonpath 得到结果。用户负责调用`json_decref`释放返回节点。同上述 API 一样，返回的节点可能指向 JSON 树中的节点，`json_path_get_distinct`的返回值区分单个节点与集合。

## 语法、语义

详细见语法文档。

### 与常见的 JSONPath 的差异

支持以下操作符：

单目 : `!  + - & * ~`

双目 : `+ - * / % & ^ | && || << >> == != < > <= >= ++ =~`

它们大部分与 C 语言一样，操作符优先级和 C 语言相同，语义也类似 C 语言。 `++`优先级与`+`相同。`=~`与`==`优先级相同，仅当开启正则表达式特性时存在。

使用`.#`来获得节点的成员数量。

Janssonpath在索引时可以省略最初的节点，并默认为 `@` （当前节点），最外层的 `@` 等同于 `$` （根节点）。举例来说 `.book[.#/2]` 相当于 `@.book[@.#/2]` 也即 `$.book[@.#/2]`。

### 与版本 1.X 的差异

不再支持 `$.book.1`，`$[book]`这两种语法特性。它们并没有很大的价值，而且前者会造成需要混杂词法、句法分析来解决的歧义（可分词为 {$} {.} {book} {.1}），后者与新引入的自定义变量特性、表达式不需要括号的特性冲突。

不再支持`$..[anything]`。这种语法导致通常用法的`$..anything`与常见的 JSONPath 实现不同。在 2.X 的版本中可以用`$..*[anything]`等价地实现。

在表达式中的大部分位置可以随意地使用空格。

版本 2.X 同样允许嵌套任意层数的 JSONPath 表达式，在 1.X 中 $ 表示的根节点是绝对的，指向最初传入的根节点，而 2.X 中，`($.book_store[1].book++$.book_store[2].book)[($.#/2)]`，其中`$.#`的$指代的是`($.book_store[1].book++$.book_store[2].book)`。这是为了用户将 Janssonpath 作为子语言使用，写出`My_variable.book[?(@.price>$.average_price)]`这样的表达式时不至于得出令人惊讶的结果。

不再无条件地将非 ASCII 区域的 char 视作标识符的一部分。按环境的区域设置和用户是否容忍编码错误的设置，2.X 会在遭遇错误编码时给出错误，可能会放弃编译。

### 与常见的 JSONPath 和版本 1.X 都不同的地方

常见的 JSONPath 实现中除了索引外的其他运算往往只用来作为过滤器，或者作为括号索引中的表达式使用。为了语法的一致性， Janssonpath 2.X 允许最外层的表达式为任意运算，因此以下表达式都是合法的 Janssonpath 表达式：

```jsonpath
$.index+1
1+1
-$.index
!$.is_best_json_node
```

Janssonpath 中，`$.store[($.best_store_id)]`和`$.books[?(@.price>$.average_price)]`中的括号不再必要。而且，现在`$.books[start:end]`中的start、end都可以为任意的表达式，且两者可以随意省略。举例来说`$.books[@.#/2:]`可以选取 books 后半部分的成员（数量向上取整）。