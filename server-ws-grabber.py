# WS client example

import asyncio
import websockets

async def hello():
    uri = "ws://localhost:9002"
    async with websockets.connect(uri, max_size=1_000_000_000) as websocket:
        data = await websocket.recv()
        with open("test.txt", "wb") as test_file:
            test_file.write(data)

asyncio.get_event_loop().run_until_complete(hello())
