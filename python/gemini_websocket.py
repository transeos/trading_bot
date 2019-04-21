# askmarketdata.py
# Mohammad Usman
#
# A simple example showing how the BaseWebSocket class can be modified.
# AsksWebSocket will print the latest 100 ask orders and then close the
# connection

import sys
sys.path.insert(0, '..')
from gemini.basewebsocket import BaseWebSocket
from collections import deque
import json
import time

class AsksWebSocket(BaseWebSocket):
    def __init__(self, base_url):
        super().__init__(base_url)
        self._asks = deque(maxlen=100)

    def on_open(self):
        print('--Subscribed to asks orders!--\n')

    def on_message(self, msg):
        try:
            event = msg['events'][0]
            #if event['type'] == 'trade': # and event['makerSide'] == 'ask':
                #print(msg)
            print(json.dumps(msg, indent=4, sort_keys=True))
            sys.exit(0)
        except KeyError as e:
            pass


if __name__ == '__main__':
    wsClient = AsksWebSocket('wss://api.gemini.com/v1/marketdata/btcusd')
    wsClient.start()
    while True:
        if False:
            wsClient.close()
            break
        time.sleep(1)