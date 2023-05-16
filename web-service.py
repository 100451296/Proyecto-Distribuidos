from flask import Flask, request

app = Flask(__name__)

@app.route('/normal_text', methods=['POST'])
def normal_text():
    message = request.form.get('message', '')
    formatted_message = ' '.join(message.split())
    return formatted_message

if __name__ == '__main__':
    app.run(debug=True, port=4222)
    #app.normal_text("hola       buenas      !!")