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
    "/api/v2/getAddressInformation": {
      "get": {
        "tags": [
          "accounts"
        ],
        "summary": "Get Address Information",
        "description": "Get basic information about the address: balance, code, data, last_transaction_id.",
        "operationId": "get_address_information_getAddressInformation_get",
        "parameters": [
          {
            "description": "Identifier of target TON account in any form.",
            "required": true,
            "schema": {
              "type": "string",
              "title": "Address",
              "description": "Identifier of target TON account in any form."
            },
            "name": "address",
            "in": "query"
          },
          {
            "required": false,
            "schema": {
              "type": "integer",
              "title": "Masterchain block seqno"
            },
            "name": "seqno",
            "in": "query"
          }
        ],
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
    },
    "/api/v2/getExtendedAddressInformation": {
      "get": {
        "tags": [
          "accounts"
        ],
        "summary": "Get Extended Address Information",
        "description": "Similar to previous one but tries to parse additional information for known contract types. This method is based on tonlib's function *getAccountState*. For detecting wallets we recommend to use *getWalletInformation*.",
        "operationId": "get_extended_address_information_getExtendedAddressInformation_get",
        "parameters": [
          {
            "description": "Identifier of target TON account in any form.",
            "required": true,
            "schema": {
              "type": "string",
              "title": "Address",
              "description": "Identifier of target TON account in any form."
            },
            "name": "address",
            "in": "query"
          },
          {
            "required": false,
            "schema": {
              "type": "integer",
              "title": "Masterchain block seqno"
            },
            "name": "seqno",
            "in": "query"
          }
        ],
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
    },
    "/api/v2/getWalletInformation": {
      "get": {
        "tags": [
          "accounts"
        ],
        "summary": "Get Wallet Information",
        "description": "Retrieve wallet information. This method parses contract state and currently supports more wallet types than getExtendedAddressInformation: simple wallet, standart wallet, v3 wallet, v4 wallet.",
        "operationId": "get_wallet_information_getWalletInformation_get",
        "parameters": [
          {
            "description": "Identifier of target TON account in any form.",
            "required": true,
            "schema": {
              "type": "string",
              "title": "Address",
              "description": "Identifier of target TON account in any form."
            },
            "name": "address",
            "in": "query"
          },
          {
            "required": false,
            "schema": {
              "type": "integer",
              "title": "Masterchain block seqno"
            },
            "name": "seqno",
            "in": "query"
          }
        ],
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
    },
    "/api/v2/getAddressBalance": {
      "get": {
        "tags": [
          "accounts"
        ],
        "summary": "Get Address Balance",
        "description": "Get balance (in nanotons) of a given address.",
        "operationId": "get_address_balance_getAddressBalance_get",
        "parameters": [
          {
            "description": "Identifier of target TON account in any form.",
            "required": true,
            "schema": {
              "type": "string",
              "title": "Address",
              "description": "Identifier of target TON account in any form."
            },
            "name": "address",
            "in": "query"
          },
          {
            "required": false,
            "schema": {
              "type": "integer",
              "title": "Masterchain block seqno"
            },
            "name": "seqno",
            "in": "query"
          }
        ],
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
    },
    "/api/v2/getAddressState": {
      "get": {
        "tags": [
          "accounts"
        ],
        "summary": "Get Address",
        "description": "Get state of a given address. State can be either *unitialized*, *active* or *frozen*.",
        "operationId": "get_address_getAddressState_get",
        "parameters": [
          {
            "description": "Identifier of target TON account in any form.",
            "required": true,
            "schema": {
              "type": "string",
              "title": "Address",
              "description": "Identifier of target TON account in any form."
            },
            "name": "address",
            "in": "query"
          },
          {
            "required": false,
            "schema": {
              "type": "integer",
              "title": "Masterchain block seqno"
            },
            "name": "seqno",
            "in": "query"
          }
        ],
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
    },
    "/api/v2/detectAddress": {
      "get": {
        "tags": [
          "utils"
        ],
        "summary": "Detect Address",
        "description": "Get all possible address forms.",
        "operationId": "detect_address_detectAddress_get",
        "parameters": [
          {
            "description": "Identifier of target TON account in any form.",
            "required": true,
            "schema": {
              "type": "string",
              "title": "Address",
              "description": "Identifier of target TON account in any form."
            },
            "name": "address",
            "in": "query"
          }
        ],
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
    },
    "/api/v2/packAddress": {
      "get": {
        "tags": [
          "utils"
        ],
        "summary": "Pack Address",
        "description": "Convert an address from raw to human-readable format.",
        "operationId": "pack_address_packAddress_get",
        "parameters": [
          {
            "description": "Identifier of target TON account in raw form.",
            "required": true,
            "schema": {
              "type": "string",
              "title": "Address",
              "description": "Identifier of target TON account in raw form."
            },
            "example": "0:83DFD552E63729B472FCBCC8C45EBCC6691702558B68EC7527E1BA403A0F31A8",
            "name": "address",
            "in": "query"
          }
        ],
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
    },
    "/api/v2/unpackAddress": {
      "get": {
        "tags": [
          "utils"
        ],
        "summary": "Unpack Address",
        "description": "Convert an address from human-readable to raw format.",
        "operationId": "unpack_address_unpackAddress_get",
        "parameters": [
          {
            "description": "Identifier of target TON account in user-friendly form",
            "required": true,
            "schema": {
              "type": "string",
              "title": "Address",
              "description": "Identifier of target TON account in user-friendly form"
            },
            "example": "EQCD39VS5jcptHL8vMjEXrzGaRcCVYto7HUn4bpAOg8xqB2N",
            "name": "address",
            "in": "query"
          }
        ],
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
    },
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
    },
    "/api/v2/getMasterchainBlockSignatures": {
      "get": {
        "tags": [
          "blocks"
        ],
        "summary": "Get Masterchain Block Signatures",
        "description": "Get up-to-date masterchain state.",
        "operationId": "get_masterchain_block_signatures_getMasterchainBlockSignatures_get",
        "parameters": [
          {
            "required": true,
            "schema": {
              "type": "integer",
              "title": "Seqno"
            },
            "name": "seqno",
            "in": "query"
          }
        ],
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
    },
    "/api/v2/lookupBlock": {
      "get": {
        "tags": [
          "blocks"
        ],
        "summary": "Lookup Block",
        "description": "Look up block by either *seqno*, *lt* or *unixtime*.",
        "operationId": "lookup_block_lookupBlock_get",
        "parameters": [
          {
            "description": "Workchain id to look up block in",
            "required": true,
            "schema": {
              "type": "integer",
              "title": "Workchain",
              "description": "Workchain id to look up block in"
            },
            "name": "workchain",
            "in": "query"
          },
          {
            "description": "Shard id to look up block in",
            "required": true,
            "schema": {
              "type": "integer",
              "title": "Shard",
              "description": "Shard id to look up block in"
            },
            "name": "shard",
            "in": "query"
          },
          {
            "description": "Block's height",
            "required": false,
            "schema": {
              "type": "integer",
              "title": "Seqno",
              "description": "Block's height"
            },
            "name": "seqno",
            "in": "query"
          },
          {
            "description": "Block's logical time",
            "required": false,
            "schema": {
              "type": "integer",
              "title": "Lt",
              "description": "Block's logical time"
            },
            "name": "lt",
            "in": "query"
          },
          {
            "description": "Block's unixtime",
            "required": false,
            "schema": {
              "type": "integer",
              "title": "Unixtime",
              "description": "Block's unixtime"
            },
            "name": "unixtime",
            "in": "query"
          }
        ],
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
      "name": "utils",
      "description": "Some useful methods for conversion"
    },
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
