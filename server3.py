import openai
from flask import Flask, request, jsonify

app = Flask(__name__)

# Biến toàn cục lưu dữ liệu nhiệt độ, độ ẩm
latest_data = {"temperature": 35, "humidity": 80}

# Biến toàn cục lưu thông tin điện năng
latest_electrical_data = {
    "voltage": None,
    "current": None,
    "power": None,
    "energy": None,
    "frequency": None,
    "power_factor": None
}

# Cấu hình OpenAI API Key
client = openai.OpenAI(api_key="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")


@app.route('/data', methods=['POST'])
def receive_data():
    """Nhận dữ liệu từ ESP32 và cập nhật biến toàn cục"""
    global latest_data  

    data = request.get_json()
    if not data:
        return jsonify({"error": "Không nhận được dữ liệu"}), 400

    latest_data["temperature"] = data.get("temperature")
    latest_data["humidity"] = data.get("humidity")

    print(f"Nhiệt độ: {latest_data['temperature']}°C, Độ ẩm: {latest_data['humidity']}%")

    return jsonify({"message": "Dữ liệu nhận thành công!"}), 200


@app.route('/latest', methods=['GET'])
def get_latest_data():
    """Trả về dữ liệu nhiệt độ và độ ẩm mới nhất"""
    if latest_data["temperature"] is None or latest_data["humidity"] is None:
        return jsonify({"message": "Chưa có dữ liệu"}), 404

    return jsonify(latest_data), 200


@app.route('/ask', methods=['POST'])
def ask_weather_chatgpt():
    """Gửi câu hỏi về thời tiết cho OpenAI API"""
    try:
        temperature = latest_data.get("temperature")
        humidity = latest_data.get("humidity")

        print(latest_data)
        if temperature is None or humidity is None:
            return jsonify({"error": "Information missing"}), 400

        question = f"trong nhà tôi hiện nhiệt độ là {temperature}°C và độ ẩm {humidity}%. bạn có đề xuất nào cho tôi không? <15-30 word limit>"

        response = client.chat.completions.create(
            model="gpt-3.5-turbo",
            messages=[{"role": "user", "content": question}]
        )
        answer = response.choices[0].message.content
        print(answer)

        return jsonify({"response": answer}), 200

    except Exception as e:
        return jsonify({"error": str(e)}), 500


@app.route('/power-data', methods=['POST'])
def receive_power_data():
    """Nhận dữ liệu điện năng từ ESP32"""
    global latest_electrical_data  

    data = request.get_json()
    if not data:
        return jsonify({"error": "Không nhận được dữ liệu"}), 400

    latest_electrical_data["voltage"] = data.get("voltage", 0)
    latest_electrical_data["current"] = data.get("current", 0)
    latest_electrical_data["power"] = data.get("power", 0)
    latest_electrical_data["energy"] = data.get("energy", 0)
    latest_electrical_data["frequency"] = data.get("frequency", 0)
    latest_electrical_data["power_factor"] = data.get("power_factor", 0)

    print(f"Voltage: {latest_electrical_data['voltage']}V, Current: {latest_electrical_data['current']}A, "
          f"Power: {latest_electrical_data['power']}W, Energy: {latest_electrical_data['energy']}kWh, "
          f"Frequency: {latest_electrical_data['frequency']}Hz, Power Factor: {latest_electrical_data['power_factor']}")

    return jsonify({"message": "Dữ liệu điện nhận thành công!"}), 200


@app.route('/power-latest', methods=['GET'])
def get_latest_power_data():
    """Trả về dữ liệu điện năng mới nhất"""
    if all(value is None for value in latest_electrical_data.values()):
        return jsonify({"message": "Chưa có dữ liệu điện"}), 404

    return jsonify(latest_electrical_data), 200


@app.route('/ask-energy', methods=['POST'])
def ask_energy_chatgpt():
    """Gửi câu hỏi về điện năng cho OpenAI API"""
    try:
        voltage = latest_electrical_data.get("voltage")
        current = latest_electrical_data.get("current")
        power = latest_electrical_data.get("power")
        energy = latest_electrical_data.get("energy")
        frequency = latest_electrical_data.get("frequency")
        power_factor = latest_electrical_data.get("power_factor")

        if None in [voltage, current, power, energy, frequency, power_factor]:
            return jsonify({"error": "Missing electrical data"}), 400

        question = (f"The electrical parameters are Voltage: {voltage}V, Current: {current}A, "
                    f"Power: {power}W, Energy: {energy}kWh, Frequency: {frequency}Hz, Power Factor: {power_factor}. "
                    f"Is there any energy-saving recommendation? <15-30 word limit>")

        response = client.chat.completions.create(
            model="gpt-3.5-turbo",
            messages=[{"role": "user", "content": question}]
        )

        answer = response.choices[0].message.content

        return jsonify({"response": answer}), 200

    except Exception as e:
        return jsonify({"error": str(e)}), 500


if __name__ == '__main__':
    app.run(host="0.0.0.0", port=5000, debug=True)
