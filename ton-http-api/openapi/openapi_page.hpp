#pragma once
#include <string>

namespace ton_http::openapi {
std::string GetOpenApiPage() {
    return R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <meta name="description" content="SwaggerUI" />
    <title>SwaggerUI</title>
    <link rel="stylesheet" href="https://unpkg.com/swagger-ui-dist@5.11.0/swagger-ui.css" />
</head>
<body>
<div id="swagger-ui"></div>
<script src="https://unpkg.com/swagger-ui-dist@5.11.0/swagger-ui-bundle.js" crossorigin></script>
<script src="https://unpkg.com/swagger-ui-dist@5.11.0/swagger-ui-standalone-preset.js" crossorigin></script>
<script>
    window.onload = () => {
        window.ui = SwaggerUIBundle({
            url: '/api/v2/openapi.json',
            layout: "BaseLayout",
            deepLinking: true,
            dom_id: '#swagger-ui',
            plugins: [SwaggerUIBundle.plugins.DownloadURL],
            defaultModelRendering: "example",
            docExpansion:"list",
            tryItOutEnabled:true,
            showMutatedRequest:false,
            presets: [
                SwaggerUIBundle.presets.apis,
                SwaggerUIStandalonePreset
            ],
            syntaxHighlight: {"activate":true,"theme":"agate"}
        });
    };
</script>
</body>
</html>
)";
}
std::string GetOpenApiJson() {
    return R"({
  "openapi": "3.1.0",
  "info": {
    "title": "TON HTTP API C++",
    "description": "\nThis API enables HTTP access to TON blockchain - getting accounts and wallets information, looking up blocks and transactions, sending messages to the blockchain, calling get methods of smart contracts, and more.\n\nIn addition to REST API, all methods are available through [JSON-RPC endpoint](#json%20rpc)  with `method` equal to method name and `params` passed as a dictionary.\n\nThe response contains a JSON object, which always has a boolean field `ok` and either `error` or `result`. If `ok` equals true, the request was successful and the result of the query can be found in the `result` field. In case of an unsuccessful request, `ok` equals false and the error is explained in the `error`.\n\nAPI Key should be sent either as `api_key` query parameter or `X-API-Key` header.\n",
    "version": "2.1.0"
  },
  "paths": {
    "/api/v2/getMasterchainInfo": {
      "get": {
        "tags": [
          "blocks"
        ],
        "summary": "Get Masterchain Info",
        "description": "Get up-to-date masterchain state.",
        "operationId": "get_masterchain_info_getMasterchainInfo_get",
        "responses": {
          "200": {
            "description": "Successful Response",
            "content": {
              "application/json": {
                "schema": {
                  "$ref": "#/components/schemas/TonResponse"
                }
              }
            }
          },
          "422": {
            "description": "Validation Error"
          },
          "504": {
            "description": "Lite Server Timeout"
          }
        },
        "security": [
          {
            "APIKeyHeader": []
          },
          {
            "APIKeyQuery": []
          }
        ]
      }
    }
  },
  "components": {
    "schemas": {
      "TonRequestJsonRPC": {
        "properties": {
          "method": {
            "type": "string",
            "title": "Method"
          },
          "params": {
            "type": "object",
            "title": "Params",
            "default": {

            }
          },
          "id": {
            "type": "string",
            "title": "Id"
          },
          "jsonrpc": {
            "type": "string",
            "title": "Jsonrpc"
          }
        },
        "type": "object",
        "required": [
          "method"
        ],
        "title": "TonRequestJsonRPC"
      },
      "TonResponse": {
        "properties": {
          "ok": {
            "type": "boolean",
            "title": "Ok"
          },
          "result": {
            "anyOf": [
              {
                "type": "string"
              },
              {
                "items": {

                },
                "type": "array"
              },
              {
                "type": "object"
              }
            ],
            "title": "Result"
          },
          "error": {
            "type": "string",
            "title": "Error"
          },
          "code": {
            "type": "integer",
            "title": "Code"
          }
        },
        "type": "object",
        "required": [
          "ok"
        ],
        "title": "TonResponse"
      }
    },
    "securitySchemes": {
      "APIKeyHeader": {
        "type": "apiKey",
        "in": "header",
        "name": "X-API-Key"
      },
      "APIKeyQuery": {
        "type": "apiKey",
        "in": "query",
        "name": "api_key"
      }
    }
  },
  "tags": [
    {
      "name": "accounts",
      "description": "Information about accounts."
    },
    {
      "name": "blocks",
      "description": "Information about blocks."
    },
    {
      "name": "transactions",
      "description": "Fetching and locating transactions."
    },
    {
      "name": "get config",
      "description": "Get blockchain config"
    },
    {
      "name": "run method",
      "description": "Run get method of smart contract."
    },
    {
      "name": "send",
      "description": "Send data to blockchain."
    },
    {
      "name": "json rpc",
      "description": "JSON-RPC endpoint."
    }
  ]
})";
}
}
