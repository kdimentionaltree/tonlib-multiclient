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
  "servers": [
    {
      "url": "/api/v2"
    }
  ],
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
    "/api/v2/getTokenData": {
      "get": {
        "tags": [
          "accounts"
        ],
        "summary": "Get Token Data",
        "description": "Get NFT or Jetton information.",
        "operationId": "get_token_data_getTokenData_get",
        "parameters": [
          {
            "description": "Address of NFT collection/item or Jetton master/wallet smart contract",
            "required": true,
            "schema": {
              "type": "string",
              "title": "Address",
              "description": "Address of NFT collection/item or Jetton master/wallet smart contract"
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
    "/api/v2/detectHash": {
      "get": {
        "tags": [
          "utils"
        ],
        "summary": "Detect Hash",
        "description": "Get all possible hash forms.",
        "operationId": "detect_address_detectHash_get",
        "parameters": [
          {
            "description": "The 256 bit hash in any form.",
            "required": true,
            "schema": {
              "type": "string",
              "title": "Hash",
              "description": "The 256 bit hash in any form."
            },
            "name": "hash",
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
    "/api/v2/getShardBlockProof": {
      "get": {
        "tags": [
          "blocks"
        ],
        "summary": "Get Shard Block Proof",
        "description": "Get merkle proof of shardchain block.",
        "operationId": "get_shard_block_proof_getShardBlockProof_get",
        "parameters": [
          {
            "description": "Block workchain id",
            "required": true,
            "schema": {
              "type": "integer",
              "title": "Workchain",
              "description": "Block workchain id"
            },
            "name": "workchain",
            "in": "query"
          },
          {
            "description": "Block shard id",
            "required": true,
            "schema": {
              "type": "integer",
              "title": "Shard",
              "description": "Block shard id"
            },
            "name": "shard",
            "in": "query"
          },
          {
            "description": "Block seqno",
            "required": true,
            "schema": {
              "type": "integer",
              "title": "Seqno",
              "description": "Block seqno"
            },
            "name": "seqno",
            "in": "query"
          },
          {
            "description": "Seqno of masterchain block starting from which proof is required. If not specified latest masterchain block is used.",
            "required": false,
            "schema": {
              "type": "integer",
              "title": "From Seqno",
              "description": "Seqno of masterchain block starting from which proof is required. If not specified latest masterchain block is used."
            },
            "name": "from_seqno",
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
    "/api/v2/getConsensusBlock": {
      "get": {
        "tags": [
          "blocks"
        ],
        "summary": "Get Consensus Block",
        "description": "Get consensus block and its update timestamp.",
        "operationId": "get_consensus_block_getConsensusBlock_get",
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
    },
    "/api/v2/getShards": {
      "get": {
        "tags": [
          "blocks"
        ],
        "summary": "Get Shards",
        "description": "Get shards information.",
        "operationId": "get_shards_shards_get",
        "parameters": [
          {
            "description": "Masterchain seqno to fetch shards of.",
            "required": true,
            "schema": {
              "type": "integer",
              "title": "Seqno",
              "description": "Masterchain seqno to fetch shards of."
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
    "/api/v2/getBlockHeader": {
      "get": {
        "tags": [
          "blocks"
        ],
        "summary": "Get Block Header",
        "description": "Get metadata of a given block.",
        "operationId": "get_block_header_getBlockHeader_get",
        "parameters": [
          {
            "required": true,
            "schema": {
              "type": "integer",
              "title": "Workchain"
            },
            "name": "workchain",
            "in": "query"
          },
          {
            "required": true,
            "schema": {
              "type": "integer",
              "title": "Shard"
            },
            "name": "shard",
            "in": "query"
          },
          {
            "required": true,
            "schema": {
              "type": "integer",
              "title": "Seqno"
            },
            "name": "seqno",
            "in": "query"
          },
          {
            "required": false,
            "schema": {
              "type": "string",
              "title": "Root Hash"
            },
            "name": "root_hash",
            "in": "query"
          },
          {
            "required": false,
            "schema": {
              "type": "string",
              "title": "File Hash"
            },
            "name": "file_hash",
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
    "/api/v2/getOutMsgQueueSizes": {
      "get": {
        "tags": [
          "blocks"
        ],
        "summary": "Get Out Msg Queue Sizes",
        "description": "Get info with current sizes of messages queues by shards.",
        "operationId": "get_out_msg_queue_sizes_getOutMsgQueueSizes_get",
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
    "/api/v2/getBlockTransactions": {
      "get": {
        "tags": [
          "transactions"
        ],
        "summary": "Get Block Transactions",
        "description": "Get transactions of the given block.",
        "operationId": "get_block_transactions_getBlockTransactions_get",
        "parameters": [
          {
            "required": true,
            "schema": {
              "type": "integer",
              "title": "Workchain"
            },
            "name": "workchain",
            "in": "query"
          },
          {
            "required": true,
            "schema": {
              "type": "integer",
              "title": "Shard"
            },
            "name": "shard",
            "in": "query"
          },
          {
            "required": true,
            "schema": {
              "type": "integer",
              "title": "Seqno"
            },
            "name": "seqno",
            "in": "query"
          },
          {
            "required": false,
            "schema": {
              "type": "string",
              "title": "Root Hash"
            },
            "name": "root_hash",
            "in": "query"
          },
          {
            "required": false,
            "schema": {
              "type": "string",
              "title": "File Hash"
            },
            "name": "file_hash",
            "in": "query"
          },
          {
            "required": false,
            "schema": {
              "type": "integer",
              "title": "After Lt"
            },
            "name": "after_lt",
            "in": "query"
          },
          {
            "required": false,
            "schema": {
              "type": "string",
              "title": "After Hash"
            },
            "name": "after_hash",
            "in": "query"
          },
          {
            "required": false,
            "schema": {
              "type": "integer",
              "title": "Count",
              "default": 40
            },
            "name": "count",
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
    "/api/v2/getBlockTransactionsExt": {
      "get": {
        "tags": [
          "transactions"
        ],
        "summary": "Get Block Transactions Ext",
        "description": "Get transactions of the given block.",
        "operationId": "get_block_transactions_ext_getBlockTransactionsExt_get",
        "parameters": [
          {
            "required": true,
            "schema": {
              "type": "integer",
              "title": "Workchain"
            },
            "name": "workchain",
            "in": "query"
          },
          {
            "required": true,
            "schema": {
              "type": "integer",
              "title": "Shard"
            },
            "name": "shard",
            "in": "query"
          },
          {
            "required": true,
            "schema": {
              "type": "integer",
              "title": "Seqno"
            },
            "name": "seqno",
            "in": "query"
          },
          {
            "required": false,
            "schema": {
              "type": "string",
              "title": "Root Hash"
            },
            "name": "root_hash",
            "in": "query"
          },
          {
            "required": false,
            "schema": {
              "type": "string",
              "title": "File Hash"
            },
            "name": "file_hash",
            "in": "query"
          },
          {
            "required": false,
            "schema": {
              "type": "integer",
              "title": "After Lt"
            },
            "name": "after_lt",
            "in": "query"
          },
          {
            "required": false,
            "schema": {
              "type": "string",
              "title": "After Hash"
            },
            "name": "after_hash",
            "in": "query"
          },
          {
            "required": false,
            "schema": {
              "type": "integer",
              "title": "Count",
              "default": 40
            },
            "name": "count",
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
    "/api/v2/getTransactions": {
      "get": {
        "tags": [
          "transactions"
        ],
        "summary": "Get Transactions",
        "description": "Get transaction history of a given address.",
        "operationId": "get_transactions_getTransactions_get",
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
            "description": "Maximum number of transactions in response.",
            "required": false,
            "schema": {
              "type": "integer",
              "maximum": 100,
              "exclusiveMinimum": 0,
              "title": "Limit",
              "description": "Maximum number of transactions in response.",
              "default": 10
            },
            "name": "limit",
            "in": "query"
          },
          {
            "description": "Logical time of transaction to start with, must be sent with *hash*.",
            "required": false,
            "schema": {
              "type": "integer",
              "title": "Lt",
              "description": "Logical time of transaction to start with, must be sent with *hash*."
            },
            "name": "lt",
            "in": "query"
          },
          {
            "description": "Hash of transaction to start with, in *base64* or *hex* encoding , must be sent with *lt*.",
            "required": false,
            "schema": {
              "type": "string",
              "title": "Hash",
              "description": "Hash of transaction to start with, in *base64* or *hex* encoding , must be sent with *lt*."
            },
            "name": "hash",
            "in": "query"
          },
          {
            "description": "Logical time of transaction to finish with (to get tx from *lt* to *to_lt*).",
            "required": false,
            "schema": {
              "type": "integer",
              "title": "To Lt",
              "description": "Logical time of transaction to finish with (to get tx from *lt* to *to_lt*).",
              "default": 0
            },
            "name": "to_lt",
            "in": "query"
          },
          {
            "description": "By default getTransaction request is processed by any available liteserver. If *archival=true* only liteservers with full history are used.",
            "required": false,
            "schema": {
              "type": "boolean",
              "title": "Archival",
              "description": "By default getTransaction request is processed by any available liteserver. If *archival=true* only liteservers with full history are used.",
              "default": false
            },
            "name": "archival",
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
    "/api/v2/getTransactionsV2": {
      "get": {
        "tags": [
          "transactions"
        ],
        "summary": "Get Transactions V2",
        "description": "Get transaction history of a given address.",
        "operationId": "get_transactions_getTransactionsv2_get",
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
            "description": "Maximum number of transactions in response.",
            "required": false,
            "schema": {
              "type": "integer",
              "maximum": 100,
              "exclusiveMinimum": 0,
              "title": "count",
              "description": "Maximum number of transactions in response.",
              "default": 10
            },
            "name": "count",
            "in": "query"
          },
          {
            "description": "Logical time of transaction to start with, must be sent with *hash*.",
            "required": false,
            "schema": {
              "type": "integer",
              "title": "Lt",
              "description": "Logical time of transaction to start with, must be sent with *hash*."
            },
            "name": "lt",
            "in": "query"
          },
          {
            "description": "Hash of transaction to start with, in *base64* or *hex* encoding , must be sent with *lt*.",
            "required": false,
            "schema": {
              "type": "string",
              "title": "Hash",
              "description": "Hash of transaction to start with, in *base64* or *hex* encoding , must be sent with *lt*."
            },
            "name": "hash",
            "in": "query"
          },
          {
            "description": "Logical time of transaction to finish with (to get tx from *lt* to *to_lt*).",
            "required": false,
            "schema": {
              "type": "integer",
              "title": "To Lt",
              "description": "Logical time of transaction to finish with (to get tx from *lt* to *to_lt*).",
              "default": 0
            },
            "name": "to_lt",
            "in": "query"
          },
          {
            "description": "By default getTransaction request is processed by any available liteserver. If *archival=true* only liteservers with full history are used.",
            "required": false,
            "schema": {
              "type": "boolean",
              "title": "Archival",
              "description": "By default getTransaction request is processed by any available liteserver. If *archival=true* only liteservers with full history are used.",
              "default": false
            },
            "name": "archival",
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
    "/api/v2/tryLocateTx": {
      "get": {
        "tags": [
          "transactions"
        ],
        "summary": "Get Try Locate Tx",
        "description": "Locate outcoming transaction of *destination* address by incoming message.",
        "operationId": "get_try_locate_tx_tryLocateTx_get",
        "parameters": [
          {
            "required": true,
            "schema": {
              "type": "string",
              "title": "Source"
            },
            "name": "source",
            "in": "query"
          },
          {
            "required": true,
            "schema": {
              "type": "string",
              "title": "Destination"
            },
            "name": "destination",
            "in": "query"
          },
          {
            "required": true,
            "schema": {
              "type": "integer",
              "title": "Created Lt"
            },
            "name": "created_lt",
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
    "/api/v2/tryLocateResultTx": {
      "get": {
        "tags": [
          "transactions"
        ],
        "summary": "Get Try Locate Result Tx",
        "description": "Same as previous. Locate outcoming transaction of *destination* address by incoming message",
        "operationId": "get_try_locate_result_tx_tryLocateResultTx_get",
        "parameters": [
          {
            "required": true,
            "schema": {
              "type": "string",
              "title": "Source"
            },
            "name": "source",
            "in": "query"
          },
          {
            "required": true,
            "schema": {
              "type": "string",
              "title": "Destination"
            },
            "name": "destination",
            "in": "query"
          },
          {
            "required": true,
            "schema": {
              "type": "integer",
              "title": "Created Lt"
            },
            "name": "created_lt",
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
    "/api/v2/tryLocateSourceTx": {
      "get": {
        "tags": [
          "transactions"
        ],
        "summary": "Get Try Locate Source Tx",
        "description": "Locate incoming transaction of *source* address by outcoming message.",
        "operationId": "get_try_locate_source_tx_tryLocateSourceTx_get",
        "parameters": [
          {
            "required": true,
            "schema": {
              "type": "string",
              "title": "Source"
            },
            "name": "source",
            "in": "query"
          },
          {
            "required": true,
            "schema": {
              "type": "string",
              "title": "Destination"
            },
            "name": "destination",
            "in": "query"
          },
          {
            "required": true,
            "schema": {
              "type": "integer",
              "title": "Created Lt"
            },
            "name": "created_lt",
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
    "/api/v2/getConfigParam": {
      "get": {
        "tags": [
          "get config"
        ],
        "summary": "Get Config Param",
        "description": "Get config by id.",
        "operationId": "get_config_param_getConfigParam_get",
        "parameters": [
          {
            "description": "Config id",
            "required": true,
            "schema": {
              "type": "integer",
              "title": "Config Id",
              "description": "Config id"
            },
            "name": "config_id",
            "in": "query"
          },
          {
            "description": "Masterchain seqno. If not specified, latest blockchain state will be used.",
            "required": false,
            "schema": {
              "type": "integer",
              "title": "Seqno",
              "description": "Masterchain seqno. If not specified, latest blockchain state will be used."
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
    "/api/v2/getConfigAll": {
      "get": {
        "tags": [
          "get config"
        ],
        "summary": "Get Config All",
        "description": "Get cell with full config.",
        "operationId": "get_config_all_getConfigAll_get",
        "parameters": [
          {
            "description": "Masterchain seqno. If not specified, latest blockchain state will be used.",
            "required": false,
            "schema": {
              "type": "integer",
              "title": "Seqno",
              "description": "Masterchain seqno. If not specified, latest blockchain state will be used."
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
    "/api/v2/sendBoc": {
      "post": {
        "tags": [
          "send"
        ],
        "summary": "Send Boc",
        "description": "Send serialized boc file: fully packed and serialized external message to blockchain.",
        "operationId": "send_boc_sendBoc_post",
        "requestBody": {
          "content": {
            "application/json": {
              "schema": {
                "$ref": "#/components/schemas/Body_send_boc_sendBoc_post"
              }
            }
          },
          "required": true
        },
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
    "/api/v2/sendBocReturnHash": {
      "post": {
        "tags": [
          "send"
        ],
        "summary": "Send Boc Return Hash",
        "description": "Send serialized boc file: fully packed and serialized external message to blockchain. The method returns message hash.",
        "operationId": "send_boc_return_hash_sendBocReturnHash_post",
        "requestBody": {
          "content": {
            "application/json": {
              "schema": {
                "$ref": "#/components/schemas/Body_send_boc_return_hash_sendBocReturnHash_post"
              }
            }
          },
          "required": true
        },
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
    "/api/v2/sendQuery": {
      "post": {
        "tags": [
          "send"
        ],
        "summary": "Send Query",
        "description": "Send query - unpacked external message. This method takes address, body and init-params (if any), packs it to external message and sends to network. All params should be boc-serialized.",
        "operationId": "send_query_sendQuery_post",
        "requestBody": {
          "content": {
            "application/json": {
              "schema": {
                "$ref": "#/components/schemas/Body_send_query_sendQuery_post"
              }
            }
          },
          "required": true
        },
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
    "/api/v2/estimateFee": {
      "post": {
        "tags": [
          "send"
        ],
        "summary": "Estimate Fee",
        "description": "Estimate fees required for query processing. *body*, *init-code* and *init-data* accepted in serialized format (b64-encoded).",
        "operationId": "estimate_fee_estimateFee_post",
        "requestBody": {
          "content": {
            "application/json": {
              "schema": {
                "$ref": "#/components/schemas/Body_estimate_fee_estimateFee_post"
              }
            }
          },
          "required": true
        },
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
    "/api/v2/runGetMethod": {
      "post": {
        "tags": [
          "run method"
        ],
        "summary": "Run Get Method",
        "description": "Run get method on smart contract.",
        "operationId": "run_get_method_runGetMethod_post",
        "requestBody": {
          "content": {
            "application/json": {
              "schema": {
                "$ref": "#/components/schemas/Body_run_get_method_runGetMethod_post"
              }
            }
          },
          "required": true
        },
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
    "/api/v2/jsonRPC": {
      "post": {
        "tags": [
          "json rpc"
        ],
        "summary": "Jsonrpc Handler",
        "description": "All methods in the API are available through JSON-RPC protocol ([spec](https://www.jsonrpc.org/specification)).",
        "operationId": "jsonrpc_handler_jsonRPC_post",
        "requestBody": {
          "content": {
            "application/json": {
              "schema": {
                "$ref": "#/components/schemas/TonRequestJsonRPC"
              }
            }
          },
          "required": true
        },
        "responses": {
          "200": {
            "description": "Successful Response",
            "content": {
              "application/json": {
                "schema": {
                  "$ref": "#/components/schemas/DeprecatedTonResponseJsonRPC"
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
      "Body_estimate_fee_estimateFee_post": {
        "properties": {
          "address": {
            "type": "string",
            "title": "Address",
            "description": "Address in any format"
          },
          "body": {
            "type": "string",
            "title": "Body",
            "description": "b64-encoded cell with message body"
          },
          "init_code": {
            "type": "string",
            "title": "Init Code",
            "description": "b64-encoded cell with init-code",
            "default": ""
          },
          "init_data": {
            "type": "string",
            "title": "Init Data",
            "description": "b64-encoded cell with init-data",
            "default": ""
          },
          "ignore_chksig": {
            "type": "boolean",
            "title": "Ignore Chksig",
            "description": "If true during test query processing assume that all chksig operations return True",
            "default": true
          }
        },
        "type": "object",
        "required": [
          "address",
          "body"
        ],
        "title": "Body_estimate_fee_estimateFee_post"
      },
      "Body_run_get_method_runGetMethod_post": {
        "properties": {
          "address": {
            "type": "string",
            "title": "Address",
            "description": "Contract address"
          },
          "method": {
            "anyOf": [
              {
                "type": "string"
              },
              {
                "type": "integer"
              }
            ],
            "title": "Method",
            "description": "Method name or method id"
          },
          "stack": {
            "items": {
              "items": {

              },
              "type": "array"
            },
            "type": "array",
            "title": "Stack",
            "description": "Array of stack elements: `[['num',3], ['cell', cell_object], ['slice', slice_object]]`"
          },
          "seqno": {
            "type": "integer",
            "title": "Seqno",
            "description": "Seqno of masterchain block at which moment the Get Method is to be executed"
          }
        },
        "type": "object",
        "required": [
          "address",
          "method",
          "stack"
        ],
        "title": "Body_run_get_method_runGetMethod_post"
      },
      "Body_send_boc_return_hash_sendBocReturnHash_post": {
        "properties": {
          "boc": {
            "type": "string",
            "title": "Boc",
            "description": "b64 encoded bag of cells"
          }
        },
        "type": "object",
        "required": [
          "boc"
        ],
        "title": "Body_send_boc_return_hash_sendBocReturnHash_post"
      },
      "Body_send_boc_sendBoc_post": {
        "properties": {
          "boc": {
            "type": "string",
            "title": "Boc",
            "description": "b64 encoded bag of cells"
          }
        },
        "type": "object",
        "required": [
          "boc"
        ],
        "title": "Body_send_boc_sendBoc_post"
      },
      "Body_send_query_sendQuery_post": {
        "properties": {
          "address": {
            "type": "string",
            "title": "Address",
            "description": "Address in any format"
          },
          "body": {
            "type": "string",
            "title": "Body",
            "description": "b64-encoded boc-serialized cell with message body"
          },
          "init_code": {
            "type": "string",
            "title": "Init Code",
            "description": "b64-encoded boc-serialized cell with init-code",
            "default": ""
          },
          "init_data": {
            "type": "string",
            "title": "Init Data",
            "description": "b64-encoded boc-serialized cell with init-data",
            "default": ""
          }
        },
        "type": "object",
        "required": [
          "address",
          "body"
        ],
        "title": "Body_send_query_sendQuery_post"
      },
      "DeprecatedTonResponseJsonRPC": {
        "properties": {
          "ok": {
            "type": "boolean",
            "title": "Ok"
          },
          "result": {
            "title": "Result"
          },
          "error": {
            "type": "string",
            "title": "Error"
          },
          "code": {
            "type": "integer",
            "title": "Code"
          },
          "id": {
            "type": "string",
            "title": "Id"
          },
          "jsonrpc": {
            "type": "string",
            "title": "Jsonrpc",
            "default": "2.0"
          }
        },
        "type": "object",
        "required": [
          "ok",
          "id"
        ],
        "title": "DeprecatedTonResponseJsonRPC"
      },
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
