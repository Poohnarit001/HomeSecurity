import paho.mqtt.client as mqtt # นำเข้า mqtt ใช้ในการสื่อสาร
import requests
import time  # นำเข้า time module

# กำหนดค่าของ MQTT Broker และ ThingSpeak API
MQTT_BROKER = "broker.hivemq.com"  # MQTT broker ของคุณ
MQTT_PORT = 1883 # Port Public
MQTT_TOPIC = "smoke/mqtt/thingspeak"  # เปลี่ยนเป็น topic ที่ใช้จริง
THINGSPEAK_API_KEY = "6EL2YVSVXJHCEC7F"  # API Key ของ ThingSpeak
THINGSPEAK_URL = "https://api.thingspeak.com/update" # URL

# ฟังก์ชันที่ใช้เมื่อมีข้อความเข้ามาจาก MQTT
def on_message(client, userdata, message):
    try:
        payload = message.payload.decode("utf-8")
        if "=" in payload:
            value = int(payload.split('=')[1].strip())
            print(f"Received value: {value}")
            # ส่งค่าไปยัง ThingSpeak
            send_to_thingspeak(value)
        else:
            print("Invalid payload format")
    except Exception as e:
        print(f"Error processing message: {e}")

# ฟังก์ชันสำหรับส่งค่าไปยัง ThingSpeak
def send_to_thingspeak(value):
    try:
        # หน่วงเวลา 15.5 วินาทีก่อนส่งข้อมูลไปยัง ThingSpeak
        time.sleep(15.5)
        
        response = requests.get(THINGSPEAK_URL, params={
            'api_key': THINGSPEAK_API_KEY,
            'field1': value
        })
        if response.status_code == 200:
            print("Data sent to ThingSpeak successfully!")
        else:
            print(f"Failed to send data to ThingSpeak. Status code: {response.status_code}")
    except Exception as e:
        print(f"Error sending data to ThingSpeak: {e}")

# กำหนดการตั้งค่า MQTT
client = mqtt.Client()
client.on_message = on_message

client.connect(MQTT_BROKER, MQTT_PORT)
client.subscribe(MQTT_TOPIC)

# เริ่มการรับข้อมูลจาก MQTT
client.loop_forever()