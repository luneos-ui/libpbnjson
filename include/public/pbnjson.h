// @@@LICENSE
//
//      Copyright (c) 2009-2014 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// LICENSE@@@

#ifndef PJSONC_H_
#define PJSONC_H_

/*!
 @mainpage PBNJSON C
 @section PBNJSONC_INTRO C API Introduction

 This API allows easier C abstraction over the core PBNJSON library written in
C. The advantages of this API over the other APIs we have used in the past:
    - Numbers are parsed correctly regardless of how they are sent along the
wire (supports generic number format as defined by JSON spec).
    - Much faster
       - Uses a faster (& correct) parser
       - Much fewer DOM nodes created
       - Number parsing is delayed until a conversion request is made.
    - First implementation of schemas in C (more of the spec implemented than
any other implementation posted on the internet).
       - Schemas are integral to using pbnsjon.
       - Schemas define what input is accepted/reject when parsing
           - No more unnecessary checks of valid parameter passing; simply write
the schema
       - Schemas can be created from files & use mmap for minimal memory
overhead & optimal behaviour (no swapping necessary)
    - First class C++ bindings.  C++ bindings are maintained as part of this
library and treated equivalently in terms of priority.

 @subsection PBNJSONC_GEN_OVERVIEW Generating JSON & serializing to a string
 This is an example of how to create a JSON value and then convert it to a
string.

 @subsubsection PBNJSON_C_GEN_OVERVIEW_SCHEMA The schema used in the code
snippet below

@code
{
        "type" : "object", // the top-level response is a JSON object
        "properties" : { // the keys allowed by an object
                "errorCode" : { "type" : "integer" }, // errorCode is a
non-floating point number "errorText" : { "type" : "string" } // errorText is a
string value
        },
        "additionalProperties" : false // don't allow any other properties to
appear in this object
}
@endcode

 @subsubsection PBNJSON_CPP_GEN_OVERVIEW_SNIPPET The code snippet as promised:
@code
#include <pbnjson.h>
#include <stdio.h>

// ....

{
        jvalue_ref myresponse = jobject_create_var(
                {J_CSTR_TO_JVAL("errorCode"), jnumber_create_i32(5)},
                {J_CSTR_TO_JVAL("errorText"), jstring_create("This is an example
of a pbnjson object")}, J_END_OBJ_DECL
        );

        jschema_ref responseSchema = jschema_parse_file("response.schema",
NULL); const char *serialized = jvalue_tostring(myresponse, responseSchema); //
asStr lives as long as responseSchema if (serialized == NULL) {
                // some problem occured during generation
                j_release(&my_response); // remember to not leak memory
                return;
        }

        fprintf(stdout, "%s", serialized);
        j_release(&my_response);
}
@endcode

 @section PBNJSONC_OVERVIEW C API Overview
 @subsection PBNJSONC_PARSE_OVERVIEW Parsing JSON serialized within a string
 This is an example of how to take a JSON value serialized into a string, parse
it into an in-memory format, and interact with it.

 @subsubsection PBNJSONC_PARSE_OVERVIEW_SCHEMA The schema used in the code
snippet below

@code
{
        "type" : "object",
        "properties" : {
                "guess" : {
                        "type" : "number",
                        "minimum" : 1,
                        "maximum" : 10
                }
        }
}
@endcode
 @subsubsection PBNJSONC_PARSE_OVERVIEW_SNIPPET The code snippet as promised:
@code
#include <pbnjson.h>
#include <stdio.h>

// ...

{
        char *input;
        // ...	// input is initialized

        jschema_ref inputSchema = jschema_parse_file("input.schema", NULL);
        JSchemaInfo schemaInfo;
        jschema_info_init(&schemaInfo, inputSchema, NULL, NULL); // no external
refs & no error handlers jvalue_ref parsed = jdom_parse(j_cstr_to_buffer(input),
DOMOPT_NOOPT, &schemaInfo); jschema_release(&inputSchema);

        if (jis_null(parsed)) {
                // input failed to parse (this is OK since we only allow parsing
of top level elements (an object or array) j_release(&parsed); return;
        }

        // this is always guaranteed to print a number between 1 & 10.  no
additional validation necessary within the code. int guess =
jnumber_get_i32(jobject_get(parsed, J_CSTR_TO_JVAL("guess")));

        fprintf(stdout, "The guess was %d\n", guess);

        j_release(&parsed);
@endcode

 @subsubsection PBNJSONC_PARSE_OVERVIEW_SNIPPET1 The code snippet with a default
schema that accepts all input:
@code
#include <pbnjson.h>
#include <stdio.h>

// ...

{
        char *input;
        // ...	// input is initialized

        JSchemaInfo schemaInfo;
        jschema_info_init(&schemaInfo, jschema_all(), NULL, NULL); // no
external refs & no error handlers jvalue_ref parsed =
jdom_parse(j_cstr_to_buffer(input), DOMOPT_NOOPT, &schemaInfo);

        if (jis_null(parsed)) {
                // input failed to parse (this is OK since we only allow parsing
of top level elements (an object or array) j_release(&parsed); return;
        }

        // this is always guaranteed to print a number between 1 & 10.  no
additional validation necessary within the code. int guess =
jnumber_get_i32(jobject_get(parsed, J_CSTR_TO_JVAL("guess")));

        fprintf(stdout, "The guess was %d\n", guess);

        j_release(&parsed);
@endcode

@section PBNJSONC_STREAM_PARSERS Stream parsers
The library provides two stream parsers - SAX and DOM parsers. These parsers
were added to provide a mechanism to parse large json strings, when loading
entire string in memory is not possible or is too expensive. These parsers are
able to process incoming json string part by part(even byte by byte), so there
is no need to load entire string into memeory. The following examples will show
how to use it.

@subsection PBNJSONC_STREAM_PARSERS_DOM Example of usage of stream DOM parser:
@code
#include <pbnjson.h>

int main(int argc, char *argv[])
{
        const char input_json[] = "{\"number\":1, \"str\":\"asd\"}";

        // initialize schema - allow everything
        JSchemaInfo schemaInfo;
        jschema_info_init(&schemaInfo, jschema_all(), NULL, NULL);

        //create parser
        jdomparser_ref parser = jdomparser_create(&schemaInfo, 0);
        if (!parser)
                return 1;

        // Call jdomparser_feed for every part of incoming json string. It will
be done byte by byte. const char *input_json_end = input_json +
strlen(input_json); for (const char *i = input_json ; i != input_json_end ; ++i)
{

                if (!jdomparser_feed(parser, i, 1)) {
                        // Get error description
                        const char *error = jdomparser_get_error(parser);
                        break;
                }
        }

        // Finalize parsing
        if (!jdomparser_end(parser)) {
                // Get error description
                const char *error = jdomparser_get_error(parser);
        }

        // Get root of json tree
        jvalue_ref jval = jdomparser_get_result(parser);

        // Release parser
        jdomparser_release(&parser);

        // Release json object
        j_release(&jval);

        return 0;
}
@endcode

@subsection PBNJSONC_STREAM_PARSERS_SAX Example of usage of stream SAX parser:
@code
#include <pbnjson.h>

int on_null(JSAXContextRef ctxt) {
        // get pointer that was passed to jsaxparser_init
        void *orig_context = jsax_getContext(ctxt);

        // return 1 to continue parsing
        return 1;
}
int on_boolean(JSAXContextRef ctxt, bool value) {return 1;} int
on_number(JSAXContextRef ctxt, const char *number, size_t numberLen)    {return
1;} int on_string(JSAXContextRef ctxt, const char *string, size_t stringLen)
{return 1;} int on_object_start(JSAXContextRef ctxt) {return 1;} int
on_object_key(JSAXContextRef ctxt, const char *key, size_t keyLen)      {return
1;} int on_object_end(JSAXContextRef ctxt) {return 1;} int
on_array_start(JSAXContextRef ctxt)                                     {return
1;} int on_array_end(JSAXContextRef ctxt) {return 1;}

int main(int argc, char *argv[])
{
        const char input_json[] = "{\"number\":1, \"str\":\"asd\"}";

        // initialize schema - allow everything
        JSchemaInfo schemaInfo;
        jschema_info_init(&schemaInfo, jschema_all(), NULL, NULL);

        // initialize sax callback structure.
        PJSAXCallbacks callbacks;
        callbacks.m_objStart    = on_object_start;
        callbacks.m_objKey      = on_object_key;
        callbacks.m_objEnd      = on_object_end;
        callbacks.m_arrStart    = on_array_start;
        callbacks.m_arrEnd      = on_array_end;
        callbacks.m_string      = on_string;
        callbacks.m_number      = on_number;
        callbacks.m_boolean     = on_boolean;
        callbacks.m_null        = on_null;

        // Pointer that will be passed to callback. void * for simplicity
        void *callback_ctxt = NULL;

        //create parser
        jsaxparser_ref parser = jsaxparser_create(&schemaInfo, &callbacks,
callback_ctxt); if (!parser) return 1;

        // Call jsaxparser_feed for every part of incoming json string. It will
be done byte by byte. const char *input_json_end = input_json +
strlen(input_json); for (const char *i = input_json ; i != input_json_end ; ++i)
{

                if (!jsaxparser_feed(parser, i, 1)) {
                        // Get error description
                        const char *error = jsaxparser_get_error(parser);
                        break;
                }
        }

        if (!jsaxparser_end(parser)) {
                // Get error description
                const char *error = jsaxparser_get_error(parser);
        }

        // Release parser
        jsaxparser_release(&parser);

        return 0;
}

@endcode

 */
#ifdef __cplusplus
#include "pbnjson/cxx/japi.h"
#endif

#ifndef __cplusplus
#include "pbnjson/c/japi.h"
#endif
#include "pbnjson/c/jobject.h"
#include "pbnjson/c/jparse_stream.h"
#include "pbnjson/c/jschema.h"

#endif /* PJSONC_H_ */
