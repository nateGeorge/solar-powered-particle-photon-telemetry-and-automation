from sseclient import SSEClient 
import requests, re, json, datetime

def getSaveFileName():
    now = datetime.datetime.now()
    print 'saving log file as:'
    print 'recorded_messages_' + now.strftime("%Y-%m-%d %H:%M") + '.txt'
    print ''
    return 'recorded_messages_' + now.strftime("%Y-%m-%d %H:%M") + '.txt'
    
saveFile = getSaveFileName()

access_token = "YOUR ACCESS TOKEN HERE"
publish_prefix_head = "myFarm" # for subscribing to incoming messages, e.g. myFarm
publish_prefix = "myFarm/waterSystem" # e.g. myFarm/waterSystem
messages = SSEClient('https://api.spark.io/v1/events/' + publish_prefix_head + '?access_token=' + access_token)
r = requests.post('https://api.particle.io/v1/devices/events', data = {"name":publish_prefix + "/waterTankSensor/update", "data":"true", "private":"false", "ttl":"60", "access_token":access_token})
if r.json()['ok']==True:
	print 'successfully sent update request'


with open(saveFile, 'w') as record:
	for msg in messages:
		event = str(msg.event).encode('utf-8')
		data = str(msg.data).encode('utf-8')
		if re.search('jsf', event):
			dataJson = json.loads(data)
			if event == publish_prefix + '/waterTankSensor/online' and dataJson['data'] == "true":
				r = requests.post('https://api.particle.io/v1/devices/events', data = {"name":publish_prefix + "/waterTankSensor/update", "data":"true", "private":"false", "ttl":"60", "access_token":access_token})
				if r.json()['ok']==True:
					print 'successfully sent update request'
			if event == publish_prefix + '/waterTankSensor/updateConfirm':
				if dataJson['data'] == 'waiting for update':
					print 'device waiting for update...'
				if dataJson['data'] == 'not waiting for update':
					print 'device no longer waiting for update.'