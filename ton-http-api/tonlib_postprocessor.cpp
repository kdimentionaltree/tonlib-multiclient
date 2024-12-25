#include "tonlib_postprocessor.h"
#include "userver/formats/json.hpp"

namespace ton_http::core {

static const std::unordered_set<std::string> suspended_jettons{
"-1:e69571e7b9f58edfebefa297e547f36920532289dbe9ff1b76d107fcbac30104",
"-1:19bd614293ae2e6dd4fb93d7c00e50000d95b6a14784d05fff84d031e6f990db",
"-1:660ecab3220da2f1f770d016fb50669f03719cd6a5f92188e9e80399c0695fbe",
"-1:ce5f3fe4b464c70a56fc6abccbb775c48e5253c3d2c46318c6bd224d7dfefd96",
"-1:fc3d252d2b2fd4f8964348d50da8de5c56c9fd39126a4bddcbe8344cf476eca1",
"-1:d0d9b5213a7e0c03a3873c58bfe9b9a60f2ab17cd3ded31675373b958378999d",
"-1:9635d332838598e3bed341b7115d74586894f14dcb0c5c21426aa36c24ec766d",
"-1:8ca766a670559cfd65192e9eeed29c7ec5a1544c8bd9db7e5b6e4c5e663ff42f",
"-1:dae40ee38de9a0f542319bca8a0c90cee84c75cb4a8860a1efd72e89576a5fd0",
"-1:8e9735c300c1a005649a19b0a1dde75d38c2904351714a695ced416fca0d052e",
"-1:3eb1651efaaf65221a9b43cb849fdcead2c8b6758c2654d93bb80825988cefe8",
"-1:a83d524ce9f18ff4e0be3dcccc51d88a3cd7f0914aede1eb258335d7118d2a08",
"-1:8ea7ce472073d6dfc6d11be7084d384db4dec32cc3cd5ee39cb6194bc16210e0",
"-1:02a28d6b20a24a05477cfcea76c7d3e7eaa2a9ccaf21897477ea990039bb25b2",
"-1:8a49896dd0a389eaf292a3573cdfb37ff4b89c4c9965d8c83b1db8b1edbb2f20",
"-1:66d44782557077d989b3883aa7499be3933cde99d73d1bca42aed3d529fb173f",
"-1:6680f4d15366a1c672ad2f0b3eea20770260630f70582ed008a6a27275a8b3b2",
"-1:35f14297cde39e6f3e927f4feb76aed94a084f57e8abe21c5455f34da008f7b9",
"-1:d28d64b320b0a0530ab88a00c3104defdd27f4034cf03b9e5a584f8b6332c6a4",
"-1:01b573bd6dc4cc5e383d6e08af2a1e258499995903cfebfadbf6f7e39533f914",
"-1:3dc61faecbceb4fa609cbd2f3370052cf1b0299b12c6ca00cb10a8d493944bfb",
"-1:ef03ac917e6b763f85079f196b4146457019cabb1f262f678ca8182978c14fa2",
"-1:28e8033db1467cacea8b158f0f61e682de06e8a5947504c904f1f703d2be4d9e",
"-1:dfbeb774841a15254c688bbc07d7bdd993c34f8b256fc61e48a184a311865b3a",
"-1:67ce3e9105ce2643aa779e0ee6189eeda7b31bcc44617bba1b90cf40d309c208",
"-1:6b53efc00270641c1ef54c80b742f51fe4b700b2f8f40782286330172e910577",
"-1:4bca71c72a007163afa5f96fdd58df82b464f6fe38b9acddc4cce23ea7dcd611",
"-1:6131defc8334a256c2b4cf3d001ed1236bcc7552807f34364c8e7fa5f3a3502b",
"-1:3414d2793493b8fb44309550167245195dd93e3df6037b10d9d7e8eef52c6117",
"-1:df7486d3868a0fb38febd8313f67a66c5b27097472e7b2e2802474dc6da65a8a",
"-1:e5bdad2a1226615abad9265f88271c82713426993a3bd5f216c90df51f127c36",
"-1:2c3062b2a70d34e8079162a8a62e3998d947e9f921e4f02d19241360541973c3",
"-1:b17045f9f82db60c4f3a4bb1206e108def01a2e2b400df969771a7e3392555ad",
"0:9a4821d56a4fa321cf4036eb5301f1e964d33761d1a4cbbc9a0f3b13ae0d1684",
"0:52f54dc07c3d797c698eb51317400c5c3d507036e22d7cb7039e2f5f91b4f2c8",
"0:6230d2fda8d58c024247b8cd0116779872e1cbd7b9334b8a6cc296843be3465a",
"0:f1921990bcf0ed81c36cb7be49c97adee85d5df4703946b4247c09bee8461b79",
"0:57d433af4a8c768e78cde34645b5449085224c0b211684145d4444514a5b626c",
"0:08082a325d5e0d291eb4487c38182d0db474853a9846c36b5171b545f2f41eab",
"0:a3ced23f38b77551326d498de8fdbf2a6710f9b131687891c803f09f43716b8a",
"0:759b6cf34cb71967300ec403107761b1d01f960512c45f05f2537314a6bb9944",
"0:ccb433303a8b52528c2967cdcd7e9609602fd762f408a8161cb83509a0571e7a",
"0:34de8fb422016bb981681e8b7de82ff01fda402ab5591900443561d561f37c77",
"0:eccf124f399017d33b6695bb3b78380f970af7b6b9bcfa7cc57c07666ca86e81",
"0:1a242b8250d00dc0b76fa953688af6b553d1478465c55030c5835c17ee8192f6",
"0:78873be25710f5ef480865fb163c614a7db381e8b08fe58847ec8edf5b3ef077",
"0:77fb08067d071ba69a3e5e75ecbf3c2bef21e08501f4ee9e9f302fc045d2228f",
"0:973696d592e2b14f13db7381969e04abac18e1443d134125eac695bf83e5a10a",
"0:5d7e73f319b6cadfc7941d46f530764276b8ad58331b123f9f8cf7518af76bb8",
"0:ce811136b1b98f66997d940e98de2c523c50fc085feba4b97a37d9fa8b42a12f",
"0:0927afd84711fea63794c619ded2288c7479b3ec16c927f0360ed76603f6df06",
"0:b42664f2a6ac921d531524c1d6b296f7c6bb362eb772e1e27e33b431a5b4b870",
"0:6d4acd125383c243c1f8abc42aa0ad0b3f024b91c272c0bcfd5a09195e5fd3eb",
"-1:50ceebab9d128691d06488571531a858d0d783cbd44848b2e62ca6099e9d03f5",
"-1:5ec36a53dc55d2fa6f304479de7f54a9877f03bf637af7f492583ab52fb73df1",
"0:e8ce75fa386a1daf00a489851eb7441084aa83e6fb3f5d5a16930f4f588adce2",
"0:cb3503b1117458e8908ba71424a078ec648be57c1706c6685da71731c3b85dd3",
"0:22e69f616aecab212a3aa133f596fb4345ca51fbc0d755fc05a0bb525c2cd42b",
"0:495256f32bb4932ba478d8b7061712ebb9d2b132e5b9829ac6001726483843a1",
"0:9257d26e1aa4c5bf1840f1b6777ecf4ba0ef9be81ddf8e89b4906c0dbeb543e2",
"0:3cfd6e07318cac5199f32b2440ba05b1d3090f96357a82dc7fb3e7dd60670d00",
"0:00ed1c8d25c4774e0ef3fc1c176fcd5e3c3beb2f2b506a73bad9303aea648313",
"0:59ffca801ca64d630735fd09534e1b2f31f525a9e6da07083554cbc738b8f486",
"0:d8e4683041eb92670047ef67b439008f41eaea02c6513d1c7a0954f139bd8dfb",
"0:23b513f5915f0d4b6a28e8debb1b5c0638e58c309772caedf55a838a70a625dc",
"0:1c7c0347ecf1b679e4dd5f956bd9951e6f109fc09cb3042138a2b8945515378c",
"0:24bd5413b4f996618eb848b23e0ca4baa016d043426b18ec8b87ddcd9ab60f41",
"0:5f3fa1e81dbff3a9ce54dcb87cfe934cb7447b230b31bbce7c77476a99e01187",
"0:0ccb694f7476543e6ebde59f9b87467575cb5ae315d7edf97a7657364973a588",
"0:eb662dfa2a319c3bee59fba009f40a76be064a7fb27370c47811d8c4e5feea95",
"0:d3df81835a2722628eb6b846e84bcb23de194a51701661c33752b73436308c83",
"0:81797aadec88b02287d9fab623cfa8127912e31632dd1507ffb91ad79d96ae2b",
"-1:ed2cba0b988bdaa12c4a5f5b177e51d93b54c7cd2f91515214bb1fa04faef290",
"0:2d94745933217ef0f1d1c92bb95627b40b657f4616794756f90b8b6f9d3b62ea",
"0:d6b6ff8b69fcd0adbd4e22959c0ca1e8ed8bedd96a508ad94cdb7fb6924b0cae",
"0:567ccaef29948438793cbfffa4b1eac1a62276adecca6ee12160b7d91aa897e9",
"0:2ffff28b9b9d19cfd88dea7d81176786b41ecac237ded40832de592377d15226",
"0:9ef583316539952b7d1b689c24338c3485ceb46fe25ffde1316cf688be450ea4",
"0:baa3c50ddb54cbbedea3ec6e2e892d4f0772c1f0efcc77bd199a4f6e89c4633e",
"0:820891e001cb3ec10ce07fff483ca7649f0fde8f467e43e9f706d4842a2d2122",
"0:0000000000000000000000000000000000000000000000000000000000000000",
"0:42d8cdc3975887f2907a920edde6f761da733fd7468583e45e32cb4010fc4d28",
"0:eb0635b137f6667b96ae4459f9a8a359b687faae59307920867fd8b3eb42661a",
"0:f1daf90b511a49638c5ea18e2304d23ecb9e73232640e1ea66016a1bf74a0b9f",
"0:6ffb4cec37d8fe2f1b3512f6c61b47ccc1b79ea9b5a5c53cf28d5e0b13ed2e25",
"0:14cc4f8d79ff343bd74b672f751df3e239be84110f18ab258ac8853e29e6982c",
"0:fe9e1ae29e3bf40782d382bd1d4474c7c07d4c3ac80684a05725977bbc448c65",
"0:150f71678bd23a8fc5de48897ec00181121aa16a67ba3ad018d0649c293d03f5",
"0:ba002ab75403598ea7d1072ec93079292f0a90f6ac9826c0881b1d6661b420c5",
"0:8824b345224a390afa4a9fda0092b0e02e87033a80f8bf0987c4dc7f7ec1bf2e",
"0:9ee0060805c7b5a40936a8924f4acd4ecf265194a8e47eaf33b82f03edeaefe8",
"0:adaed4d7100a12a681119a2a8e9c809ef4b1f19f637cf4de011fda9d1e5ac6f9",
"0:4196b680a6cd0e65a07d43c78f427db054068bd46e35d40f9d369a7259edd7dc",
"0:62012a94a9a562ceba332d16ecce4582da41136e48693b6aa78835433cf836ff",
"0:6ed90d443bc808a0d035aa81969c752e74a7e6aa52840d3404f83c4dfbfe2c76",
"0:396efc56779faadaf25456a47d0256c421f745cac2d1d0f5c68159417fcc7985",
"0:60b8c4878c5bf96dfa0e76c86bdebb92f7299267385128ca2fa0daf11b1657be",
"0:312f4a05d7eccd82298f2ad85b0e7612e318fde2b1ed7cd44deff3ddd6bd8b34",
"0:0d5f5b9128665fc8f23011a217ad1d193aa69d72de6ad7476f2b1588c71ed242",
"0:62b5d143a14e435928df2aaec0b0edbd448c009c1cf3bdc5554a7e5e910a01f4",
"0:606eec7b5528ea2473fb07244ad6ae10b9565ba5209eb0d82fcf9a9a77795f5d",
"0:e2b0177c53b337084399f2071609d7ad766262dc0237d73213e6a07039d9946e",
"0:19a67fdeca75657fb222d9dea33ffa755113eb745f45dd2ec38ae750c9e9648f",
"0:c4d070607e43a2a877b32ed1310464ef3acf1e8adb611d0a2c0635b70660f2d7",
"0:f24f7eb392d04df4f556bd9d63a11c6121f1148023ede05da2b1af9e9d403f77",
"0:d23141b0b76e4841ddffc878c8d3717bd46f0aa7618c68c12fd93896a1329bb3",
"0:202b511caef12e2fdb02b688f770d85534eb9fde5a391883c4da09f57f065960",
"0:832aa2f9c3fe8c0fe67b00e06eb6efae47e6494ec80d5357eee3ae219fd82b63",
"0:756953321418679585e6423b266b44bade4bf1294edc719285342153c17ec048",
"0:5b8eb5c7a310f2875658181741a1080b6927315baa594758448490bc06a17ee4",
"0:be41e675dd03f496ccf134ab77f347c25ae695f78bf00bf6dfbaa9ab960d7f7c",
"0:22530a45dbbbd70a782a3617ff4d0e44ff4e9a8741b8cee4ab9df53b08b2da86",
"0:6a8958e685eda25469a03920ced451a34a2cff00cb5bd70ee52c488fe17205fd",
"0:e6c2f68ad9dfd26da3dda4e33ad1e7df3d678ab24b200d5c053c4cb1b9fb4cdf",
"0:d6b4f659805ef0bc83141d94288ee0f7bad8ae8afb16ed6b25449df7760bfee7",
"0:63f102c5d902d4d0a0cc09576ee7af3342c85ba6d5a5357dbca0785305b985fd",
"0:186ed0c264419100e5da07a5b056a173613775cd9832a75e4122ecbd9f4dbd93",
"0:9376f0c27f46c0e5d38f235fb8e2d07665f35c3068a4973be0dbb4e8be00c4a8",
"0:6c0a12df8596da476de025a47c49ebc85e8c1edae52d2100b38ee72e03d8aedf",
"0:4304a139586288967a22dce3b3c3bbe428a217e797968cfbe0c495b07a4f00c8",
"0:2f092c8b0a8db51aba8b0227dda2fe1690ec6e1900a3311c6017571749a3732c",
"0:eceb82522275740372bf32a3da6fba83d8c68990d04aab9b71c774507b9126c9",
"0:6a035ca23b47c3e1a5ccb24e214013b8ad67f3c7674da63dd4d6c763db51b2e8",
"0:dc07e998d553b4563b020dee62d3e9d38298b2c22bb852e980cc7e687354a293",
"0:c705422111d502509d7df7af92123d168ab69a13827a60b24eb8b28ba4af2e81",
"0:85dd813ece0b0250a34338ef881dc2a068db836e16d3da63c3a7a084a00fe51b",
"0:68ee815e7df6641338dc9b6a3dc9db1f6a773a1616c0e6e75dc0665cbecb4598",
"0:8670de1bd64dbe4ceadf00c3a46b7471be5c1c7150d22b470f7b39476607cbc7",
"-1:062bb40f112ffaac45f54209c9a187dc95f9a6c4b07b264664832323398e2299",
"-1:4e2b4087aa806275c114247c789da5774ed82652fd681211a9b770c52c69e772",
"-1:707cf9eff139c9e7f83df22fc4869047ff497031de94521ff2dfdd40c2eb3c46",
"-1:daec5b9b51f23c7d43e700866f21129a742f461bace98cb2c4e8f5d58fe75ee6",
"-1:9dd3bfb670c27f144544555c803057b8f5a06e467871cc50b1afbd7cf65f82db",
"0:3457022d52a7209684ac3a8e633819f4926b59dfc0d889d350e12151f63354f6",
"0:1b98c2fddf1bd44fe8810c9ace7e1891192b37bce733132f432406ee09beda53",
"0:200144a57fdce64376e61deb7995bea7f61d490b42121793cc1fba2b67cbfb95",
"0:0769ffdea3d8261cb8844691f963979baffcf8a57e0dcac0263cc7076bd4976a",
"0:0fec8cf9cefce0e363a6ae3be7d8a0060a46f91fab5806d8faa7e1ac6c8a4c29",
"0:3227ae037fb55f6fcfe39a51a7b19072180742b04580fc57d65f254453977a0a",
"0:78e7b2f646eede090f0d8604ada07c6f7b0552b0a08386ac9370be3bc509d669",
"0:7bd6ace07a1217a0c463e7cbc059a1e73408ba517114e427bde36749a9fe076b",
"0:7bb000be851906824db0dd13019427cd930e954c2e836be191995b17287e0859",
"0:5adbdef467512619ffb747e061eeddd699130716b601f89771679162a1352b7c",
"0:5d0af141e1b03b3c03f45579d3b3bf384ceff80a858b6f542a45dcabd28acb02",
"0:5fe31152cbe3045dcc14b6ad5b0ff14a0d6cd22c9e2de601e160abac3352d5c1",
"0:6677fa11f3c4eb7c019c2ab327e384f7c019db8774d65505abf2b56ad1b6139d",
"0:676963a7c0dfe6a9c0c41776910cbbcb3cb855dffbadb99ad7dc88cb74761e0b",
"0:6c9cce64fec3b39d0af559d13de077c258349e2cae9c0588b9c51c40a74917a0",
"0:52cdc190ddbeeb86ccec2d25a4759900a1f46d45ef45b78962d0e506928ea39e",
"0:7077dd1dd31dbab37c0e74e602570fb3da06d1ee140998243c53bcfc569e632c",
"0:5938393f375da0a562274e94542c69350b8b3f7309032fddad7255d0caf022cf",
"0:b50e6fc3bee08644516aad9d1190f09a4c90c06872967293f768dddffd3189d5",
"0:b9416647545b2735cdd83d161a7d412b160fe2d2c970a128b24e88c177ad536a",
"0:bb6c62ea2365cf74a189ccd9643e73a7077624c34410f6520c642656e465afa6",
"0:853a00de1b6b5bff13112794d00d8aba04f9484a6f4a4b13a3bc6c0ddf6989fb",
"0:871d8551bd6ec4aad0598ed786aefceb6ecda5899880254c9ebd1399676855df",
"0:873879b429a6462ea0cf51944774b6103be9ec1c7acb6849ab7d0f7985df0c53",
"0:8fa7da857b09b99185819ceed2e20978ff2f8d3ee19973be59849d93f1cae42b",
"0:a94e11bd39f607a7bef279a512ffde668473d582abdeac83b9d1138dcacc1602",
"0:ac2e90de1c1daaa516e6b026ffd319ac615371d4d81e62bbc7193d530eb1e40e",
"0:9343c90fc325fddabe3c80220bfe92cdfd17104fd8216728279834d890e92e5b",
"0:93ac54521edfad7e4ed6c4af591d09aa2dc8315f14caef019f0846ea818d96f4",
"0:afdbccb8f4675a2e7728725bf421274394c518fdcc433c63561423d3f2de06ba",
"0:952fe93a75e0d1e7a67ca5c6f406d8349467da8af3867dd0c20f5befc91b322f",
"0:fab767570e36711750eaba7285fbc7445a2bc9765898ff41588825e7b61baa9c",
"0:c4f7c5cdcd788fabb5676c9276338f87b77b57fce591d315134e70c2f0d39b2e",
"0:df7910c4d530e1c2de3bf020d1d10ecc0f0a9c4d485f8eac934f9f6682801a17",
"0:e28dc13250a6be42009b786960810935b81b6dd99447c96e18e2007847a30de9",
"0:ceb453ade35b246e3064645f3da185e9a031028e5c48cc3f9285600e632ceec6",
"0:f01aee2f16fd5edf09fc9a29ff8b2ece3c95de80e613150e5b6456796b964f88",
"0:b81b6a61e804bf983ffe708bf8688626d73e63020096fd34c312bef6ca05ce3f"};

// utils
std::string get_address_state(tonlib_api::object_ptr<tonlib_api::raw_fullAccountState>& state) {
  if (state->code_.empty()) {
    if (state->frozen_hash_.empty()) {
      return "uninitialized";
    } else {
      return "frozen";
    }
  }
  return "active";
}

// utils
template<typename T>
auto tl_to_json(const T& value) {
  return userver::formats::json::FromString(td::json_encode<td::string>(td::ToJson(value)));
}

inline td::Result<std::string> get_hash(const std::string& data) {
  if (data.empty()) {
    return td::Status::Error("Empty data");
  }
  auto res = vm::std_boc_deserialize(data);
  if (res.is_error()) { return res.move_as_error(); }
  return td::base64_encode(res.move_as_ok()->get_hash().as_slice());
}

inline std::string get_hash_no_error(const std::string& data) {
  return get_hash(data).move_as_ok();
}


// wallet data parsers
namespace wallets {
const std::string wallet_v1_r1 = "te6cckEBAQEARAAAhP8AIN2k8mCBAgDXGCDXCx/tRNDTH9P/0VESuvKhIvkBVBBE+RDyovgAAdMfMSDXSpbTB9QC+wDe0aTIyx/L/8ntVEH98Ik=";
const std::string wallet_v1_r2 = "te6cckEBAQEAUwAAov8AIN0gggFMl7qXMO1E0NcLH+Ck8mCBAgDXGCDXCx/tRNDTH9P/0VESuvKhIvkBVBBE+RDyovgAAdMfMSDXSpbTB9QC+wDe0aTIyx/L/8ntVNDieG8=";
const std::string wallet_v1_r3 = "te6cckEBAQEAXwAAuv8AIN0gggFMl7ohggEznLqxnHGw7UTQ0x/XC//jBOCk8mCBAgDXGCDXCx/tRNDTH9P/0VESuvKhIvkBVBBE+RDyovgAAdMfMSDXSpbTB9QC+wDe0aTIyx/L/8ntVLW4bkI=";
const std::string wallet_v2_r1 = "te6cckEBAQEAVwAAqv8AIN0gggFMl7qXMO1E0NcLH+Ck8mCDCNcYINMf0x8B+CO78mPtRNDTH9P/0VExuvKhA/kBVBBC+RDyovgAApMg10qW0wfUAvsA6NGkyMsfy//J7VShNwu2";
const std::string wallet_v2_r2 = "te6cckEBAQEAYwAAwv8AIN0gggFMl7ohggEznLqxnHGw7UTQ0x/XC//jBOCk8mCDCNcYINMf0x8B+CO78mPtRNDTH9P/0VExuvKhA/kBVBBC+RDyovgAApMg10qW0wfUAvsA6NGkyMsfy//J7VQETNeh";
const std::string wallet_v3_r1 = "te6cckEBAQEAYgAAwP8AIN0gggFMl7qXMO1E0NcLH+Ck8mCDCNcYINMf0x/TH/gjE7vyY+1E0NMf0x/T/9FRMrryoVFEuvKiBPkBVBBV+RDyo/gAkyDXSpbTB9QC+wDo0QGkyMsfyx/L/8ntVD++buA=";
const std::string wallet_v3_r2 = "te6cckEBAQEAcQAA3v8AIN0gggFMl7ohggEznLqxn3Gw7UTQ0x/THzHXC//jBOCk8mCDCNcYINMf0x/TH/gjE7vyY+1E0NMf0x/T/9FRMrryoVFEuvKiBPkBVBBV+RDyo/gAkyDXSpbTB9QC+wDo0QGkyMsfyx/L/8ntVBC9ba0=";
const std::string wallet_v4_r1 = "te6cckECFQEAAvUAART/APSkE/S88sgLAQIBIAIDAgFIBAUE+PKDCNcYINMf0x/THwL4I7vyY+1E0NMf0x/T//QE0VFDuvKhUVG68qIF+QFUEGT5EPKj+AAkpMjLH1JAyx9SMMv/UhD0AMntVPgPAdMHIcAAn2xRkyDXSpbTB9QC+wDoMOAhwAHjACHAAuMAAcADkTDjDQOkyMsfEssfy/8REhMUA+7QAdDTAwFxsJFb4CHXScEgkVvgAdMfIYIQcGx1Z70ighBibG5jvbAighBkc3RyvbCSXwPgAvpAMCD6RAHIygfL/8nQ7UTQgQFA1yH0BDBcgQEI9ApvoTGzkl8F4ATTP8glghBwbHVnupEx4w0kghBibG5juuMABAYHCAIBIAkKAFAB+gD0BDCCEHBsdWeDHrFwgBhQBcsFJ88WUAP6AvQAEstpyx9SEMs/AFL4J28ighBibG5jgx6xcIAYUAXLBSfPFiT6AhTLahPLH1Iwyz8B+gL0AACSghBkc3Ryuo41BIEBCPRZMO1E0IEBQNcgyAHPFvQAye1UghBkc3Rygx6xcIAYUATLBVjPFiL6AhLLassfyz+UEDRfBOLJgED7AAIBIAsMAFm9JCtvaiaECAoGuQ+gIYRw1AgIR6STfSmRDOaQPp/5g3gSgBt4EBSJhxWfMYQCAVgNDgARuMl+1E0NcLH4AD2ynftRNCBAUDXIfQEMALIygfL/8nQAYEBCPQKb6ExgAgEgDxAAGa3OdqJoQCBrkOuF/8AAGa8d9qJoQBBrkOuFj8AAbtIH+gDU1CL5AAXIygcVy//J0Hd0gBjIywXLAiLPFlAF+gIUy2sSzMzJcfsAyEAUgQEI9FHypwIAbIEBCNcYyFQgJYEBCPRR8qeCEG5vdGVwdIAYyMsFywJQBM8WghAF9eEA+gITy2oSyx/JcfsAAgBygQEI1xgwUgKBAQj0WfKn+CWCEGRzdHJwdIAYyMsFywJQBc8WghAF9eEA+gIUy2oTyx8Syz/Jc/sAAAr0AMntVEap808=";
const std::string wallet_v4_r2 = "te6cckECFAEAAtQAART/APSkE/S88sgLAQIBIAIDAgFIBAUE+PKDCNcYINMf0x/THwL4I7vyZO1E0NMf0x/T//QE0VFDuvKhUVG68qIF+QFUEGT5EPKj+AAkpMjLH1JAyx9SMMv/UhD0AMntVPgPAdMHIcAAn2xRkyDXSpbTB9QC+wDoMOAhwAHjACHAAuMAAcADkTDjDQOkyMsfEssfy/8QERITAubQAdDTAyFxsJJfBOAi10nBIJJfBOAC0x8hghBwbHVnvSKCEGRzdHK9sJJfBeAD+kAwIPpEAcjKB8v/ydDtRNCBAUDXIfQEMFyBAQj0Cm+hMbOSXwfgBdM/yCWCEHBsdWe6kjgw4w0DghBkc3RyupJfBuMNBgcCASAICQB4AfoA9AQw+CdvIjBQCqEhvvLgUIIQcGx1Z4MesXCAGFAEywUmzxZY+gIZ9ADLaRfLH1Jgyz8gyYBA+wAGAIpQBIEBCPRZMO1E0IEBQNcgyAHPFvQAye1UAXKwjiOCEGRzdHKDHrFwgBhQBcsFUAPPFiP6AhPLassfyz/JgED7AJJfA+ICASAKCwBZvSQrb2omhAgKBrkPoCGEcNQICEekk30pkQzmkD6f+YN4EoAbeBAUiYcVnzGEAgFYDA0AEbjJftRNDXCx+AA9sp37UTQgQFA1yH0BDACyMoHy//J0AGBAQj0Cm+hMYAIBIA4PABmtznaiaEAga5Drhf/AABmvHfaiaEAQa5DrhY/AAG7SB/oA1NQi+QAFyMoHFcv/ydB3dIAYyMsFywIizxZQBfoCFMtrEszMyXP7AMhAFIEBCPRR8qcCAHCBAQjXGPoA0z/IVCBHgQEI9FHyp4IQbm90ZXB0gBjIywXLAlAGzxZQBPoCFMtqEssfyz/Jc/sAAgBsgQEI1xj6ANM/MFIkgQEI9Fnyp4IQZHN0cnB0gBjIywXLAlAFzxZQA/oCE8tqyx8Syz/Jc/sAAAr0AMntVGliJeU=";
const std::string wallet_v5_r1 = "te6cckECFAEAAoEAART/APSkE/S88sgLAQIBIAIDAgFIBAUBAvIOAtzQINdJwSCRW49jINcLHyCCEGV4dG69IYIQc2ludL2wkl8D4IIQZXh0brqOtIAg1yEB0HTXIfpAMPpE+Cj6RDBYvZFb4O1E0IEBQdch9AWDB/QOb6ExkTDhgEDXIXB/2zzgMSDXSYECgLmRMOBw4hAPAgEgBgcCASAICQAZvl8PaiaECAoOuQ+gLAIBbgoLAgFIDA0AGa3OdqJoQCDrkOuF/8AAGa8d9qJoQBDrkOuFj8AAF7Ml+1E0HHXIdcLH4AARsmL7UTQ1woAgAR4g1wsfghBzaWduuvLgin8PAeaO8O2i7fshgwjXIgKDCNcjIIAg1yHTH9Mf0x/tRNDSANMfINMf0//XCgAK+QFAzPkQmiiUXwrbMeHywIffArNQB7Dy0IRRJbry4IVQNrry4Ib4I7vy0IgikvgA3gGkf8jKAMsfAc8Wye1UIJL4D95w2zzYEAP27aLt+wL0BCFukmwhjkwCIdc5MHCUIccAs44tAdcoIHYeQ2wg10nACPLgkyDXSsAC8uCTINcdBscSwgBSMLDy0InXTNc5MAGk6GwShAe78uCT10rAAPLgk+1V4tIAAcAAkVvg69csCBQgkXCWAdcsCBwS4lIQseMPINdKERITAJYB+kAB+kT4KPpEMFi68uCR7UTQgQFB1xj0BQSdf8jKAEAEgwf0U/Lgi44UA4MH9Fvy4Iwi1woAIW4Bs7Dy0JDiyFADzxYS9ADJ7VQAcjDXLAgkji0h8uCS0gDtRNDSAFETuvLQj1RQMJExnAGBAUDXIdcKAPLgjuLIygBYzxbJ7VST8sCN4gAQk1vbMeHXTNCon9ZI";
const std::string nominator_pool_v1 = "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";

td::Result<> parse_wallet_seqno(userver::formats::json::ValueBuilder& builder, td::Ref<vm::Cell>& data) {
  vm::CellSlice cs{vm::NoVm(), data};
  builder["seqno"] = cs.fetch_long(32);
  return td::Status::OK();
}

td::Result<> parse_wallet_v3(userver::formats::json::ValueBuilder& builder, td::Ref<vm::Cell>& data) {
  vm::CellSlice cs{vm::NoVm(), data};
  builder["seqno"] = cs.fetch_long(32);
  builder["wallet_id"] = cs.fetch_long(32);
  return td::Status::OK();
}

td::Result<> parse_wallet_v5(userver::formats::json::ValueBuilder& builder, td::Ref<vm::Cell>& data) {
  vm::CellSlice cs{vm::NoVm(), data};
  bool is_signature_allowed = cs.fetch_long(1);
  builder["seqno"] = cs.fetch_long(32);
  builder["wallet_id"] = cs.fetch_long(32);
  builder["is_signature_allowed"] = is_signature_allowed;
  return td::Status::OK();
}

static const std::map<std::string, std::pair<std::string, std::function<td::Result<>(userver::formats::json::ValueBuilder&, td::Ref<vm::Cell>&)>>> wallet_data_parsers{
{"oM/CxIruFqJx8s/AtzgtgXVs7LEBfQd/qqs7tgL2how=", {"wallet v1 r1", parse_wallet_seqno}},
{"1JAvzJ+tdGmPqONTIgpo2g3PcuMryy657gQhfBfTBiw=", {"wallet v1 r2", parse_wallet_seqno}},
{"WHzHie/xyE9G7DeX5F/ICaFP9a4k8eDHpqmcydyQYf8=", {"wallet v1 r3", parse_wallet_seqno}},
{"XJpeaMEI4YchoHxC+ZVr+zmtd+xtYktgxXbsiO7mUyk=", {"wallet v2 r1", parse_wallet_seqno}},
{"/pUw0yQ4Uwg+8u8LTCkIwKv2+hwx6iQ6rKpb+MfXU/E=", {"wallet v2 r2", parse_wallet_seqno}},
{"thBBpYp5gLlG6PueGY48kE0keZ/6NldOpCUcQaVm9YE=", {"wallet v3 r1", parse_wallet_v3}},
{"hNr6RJ+Ypph3ibojI1gHK8D3bcRSQAKl0JGLmnXS1Zk=", {"wallet v3 r2", parse_wallet_v3}},
{"ZN1UgFUixb6KnbWc6gEFzPDQh4bKeb64y3nogKjXMi0=", {"wallet v4 r1", parse_wallet_v3}},
{"/rX/aCDi/w2Ug+fg1iyBfYRniftK5YDIeIZtlZ2r1cA=", {"wallet v4 r2", parse_wallet_v3}},
{"89fKU0k97trCizgZhqhJQDy6w9LFhHea8IEGWvCsS5M=", {"wallet v5 beta", parse_wallet_v5}},
{"IINLe3KxEhR+Gy+0V7hOdNGjDwT3N9T2KmaOlVLSty8=", {"wallet v5 v1", parse_wallet_v5}},
};
}


TonlibWorkerResponse TonlibPostProcessor::process_getAddressInformation(
    const std::string& address, td::Result<tonlib_api::raw_getAccountState::ReturnType>&& res
) const {
  using namespace userver::formats::json;
  if (res.is_error()) {
    return TonlibWorkerResponse::from_tonlib_result(std::move(res));
  }

  auto result = res.move_as_ok();
  ValueBuilder builder(FromString(td::json_encode<td::string>(td::ToJson(result))));
  builder["state"] = get_address_state(result);
  if (result->balance_ < 0) {
    builder["balance"] = "0";
  }
  {
    auto r_std_address = block::StdAddress::parse(address);
    if (r_std_address.is_error()) {
      return TonlibWorkerResponse::from_error_string(r_std_address.move_as_error().to_string());
    }
    auto std_address = r_std_address.move_as_ok();
    DetectAddressResult address_detect{std_address, "unknown"};
    std::string raw_address = address_detect.to_raw_form(true);
    if (result->sync_utime_ < 1803189600 && suspended_jettons.contains(raw_address)) {
      builder["suspended"] = true;
    }
  }
  return TonlibWorkerResponse{true, nullptr, ToString(builder.ExtractValue()), std::nullopt};
}
TonlibWorkerResponse TonlibPostProcessor::process_getWalletInformation(const std::string& address, td::Result<tonlib_api::raw_getAccountState::ReturnType>&& res) const {
  using namespace userver::formats::json;
  if (res.is_error()) {
    return TonlibWorkerResponse::from_tonlib_result(std::move(res));
  }

  auto result = res.move_as_ok();
  ValueBuilder builder;
  builder["wallet"] = false;
  builder["balance"] = std::to_string(result->balance_ < 0 ? 0 : result->balance_);
  builder["account_state"] = get_address_state(result);
  if (result->last_transaction_id_) {
    builder["last_transaction_id"] = tl_to_json(result->last_transaction_id_);
  }
  if (!result->code_.empty()) {
    auto code_hash_res = get_hash(result->code_);
    if (code_hash_res.is_ok()) {
      builder["wallet"] = true;
      auto code_hash = code_hash_res.move_as_ok();
      auto parser_ = wallets::wallet_data_parsers.find(code_hash);
      if (parser_ != wallets::wallet_data_parsers.end()) {
        auto data_cell = vm::std_boc_deserialize(result->data_);
        if (data_cell.is_error()) {
          return TonlibWorkerResponse::from_error_string(data_cell.move_as_error().to_string());
        }
        auto data = data_cell.move_as_ok();
        builder["wallet_type"] = parser_->second.first;
        auto parse_result = parser_->second.second(builder, data);
        if (parse_result.is_error()) {
          return TonlibWorkerResponse::from_error_string(parse_result.move_as_error().to_string());
        }
      }
    }
  }
  return TonlibWorkerResponse{true, nullptr, ToString(builder.ExtractValue()), std::nullopt};
}
TonlibWorkerResponse TonlibPostProcessor::process_getAddressBalance(const std::string& address, td::Result<tonlib_api::raw_getAccountState::ReturnType>&& res) const {
  using namespace userver::formats::json;
  if (res.is_error()) {
    return TonlibWorkerResponse::from_tonlib_result(std::move(res));
  }

  auto result = res.move_as_ok();
  auto balance =  std::to_string(result->balance_ < 0 ? 0 : result->balance_) + "\"";
  return TonlibWorkerResponse{true, nullptr, "\"" +balance + "\"", std::nullopt};
}
TonlibWorkerResponse TonlibPostProcessor::process_getAddressState(const std::string& address, td::Result<tonlib_api::raw_getAccountState::ReturnType>&& res) const {
  using namespace userver::formats::json;
  if (res.is_error()) {
    return TonlibWorkerResponse::from_tonlib_result(std::move(res));
  }

  auto result = res.move_as_ok();
  auto state = get_address_state(result);
  return TonlibWorkerResponse{true, nullptr, "\"" + state + "\"", std::nullopt};
}
}  // namespace ton_http::core
