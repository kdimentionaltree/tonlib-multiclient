import tonlib_multiclient
import time
import threading


def thread_worker(req_id, client, request_json):
    req = tonlib_multiclient.RequestJson(
        parameters=tonlib_multiclient.RequestParameters(
            mode=tonlib_multiclient.RequestMode.Broadcast,
        ),
        request=request_json,
    )
    response = client.send_json_request(req)
    if response.is_ok():
        print(f"req: {req_id} | response: {response.move_as_ok()}")
    else:
        print(f"req: {req_id} | error: {response.error().to_string()}")


if __name__ == "__main__":
    config = tonlib_multiclient.MultiClientConfig(
        global_config_path="/code/ton/tonlib-multiclient/global-config.json",
        key_store_root="/code/ton/tonlib-multiclient/data/",
        blockchain_name="mainnet",
        reset_key_store=False,
        scheduler_threads=10,
    )

    print("init multiclient")
    tonlib_multiclient.set_verbosity_level(1)

    client = tonlib_multiclient.MultiClient(config)
    time.sleep(10)

    request_json = '{"@type":"getAccountState","account_address":{"@type":"accountAddress","account_address":"UQCD39VS5jcptHL8vMjEXrzGaRcCVYto7HUn4bpAOg8xqEBI"}}'
    threads = [
        threading.Thread(target=thread_worker, args=(i, client, request_json))
        for i in range(5)
    ]
    for thread in threads:
        thread.start()

    for thread in threads:
        thread.join()

    del client
