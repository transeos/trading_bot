import sys
import gdax
import time
import json


class MyWebsocketClient(gdax.WebsocketClient):
	def on_open(self):
		print "Connection established!"
	

	def on_message(self, msg):
		#print(json.dumps(msg, indent=4, sort_keys=True))
		if(msg['type'] == 'match'):
			try:
				if(msg['side'] == 'sell'):
					print ("\033[1;32;40m price:" + msg['price'] + " size:" + msg['size'])# + " side:" + msg['side'])
				else:
					print ("\033[1;31;40m price:" + msg['price'] + " size:" + msg['size'])
			except:
				pass


wsClient = MyWebsocketClient(url="wss://ws-feed.gdax.com", products=["BTC-USD"], message_type="subscribe", 
                                mongo_collection=None, should_print=True, auth=False, api_key="", api_secret="", api_passphrase="", channels=["matches"] )
wsClient.start()

# print json.dumps(wsClient.auth_client.get_products(),indent=4, sort_keys=True)
# wsClient.close()
# sys.exit(1)

print(wsClient.url, wsClient.products)


try:
    while True:
		time.sleep(1)


except KeyboardInterrupt:
    wsClient.close()

if wsClient.error:
    sys.exit(1)
else:
    sys.exit(0)