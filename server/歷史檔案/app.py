from flask import Flask, render_template,request
import pyrebase
from flask_wtf import FlaskForm
from wtforms import StringField, IntegerField, SubmitField, BooleanField, SelectField, TextAreaField, FloatField, RadioField
from wtforms.validators import DataRequired, NumberRange
import json
import random


config = {
    "apiKey": "apiKey",
    "authDomain": "teamtedu.firebaseapp.com",
    "databaseURL": "https://teamtedu-default-rtdb.asia-southeast1.firebasedatabase.app/",
    "storageBucket": "teamtedu.appspot.com",
    "serviceAccount": "./dbkey.json"
}

firebase = pyrebase.initialize_app(config)
db = firebase.database()
app = Flask(__name__)
app.config['SECRET_KEY'] = 'tedu'  # !必填

go_to_seat = 0
status = 'standby'
@app.route('/')
def hello():
    global go_to_seat
    return {
        #'res': db.child('table').get().val()
        'seat': go_to_seat 
    }
@app.route('/status')
def status_now():
    global status
    return {
        'status': status
    }
#座位狀況表單
@app.route('/form', methods=['GET', 'POST'])
def form():
    form1 = tabledit()  # 建立表單物件
    if form1.validate_on_submit():  # 如果表單正常發送
        try:
            data = db.child('table').child(
                f'table{form1.table_no.data}').get().val()
            # data={
            # s1: ?
            # s2: ?
            # s3: ?
            # s4: ?
            # }
            data[f's{form1.seat_no.data}'] = form1.status.data
            db.child('table').child(f'table{form1.table_no.data}').update(data)
            return render_template ("back_to_clerk.html")
        except Exception as e:
            print(e)
            print(data)
            return "error"
    # form=form  html變數名稱 = 附值
    return render_template('form.html', html_form=form1)

#點餐表單
@app.route('/dish', methods=['GET', 'POST'])
def dish():
    dish = tabledish()  # 建立表單物件
    if dish.validate_on_submit():  # 如果表單正常發送
        try:
            data = db.child('table').child(
                f'table{dish.table_no.data}').get().val()
            # data={
            # d1: ?
            # d2: ?
            # d3: ?
            # d4: ?
            # }
            data[f'd{dish.seat_no.data}'] = dish.status.data
            db.child('table').child(f'table{dish.table_no.data}').update(data)
            return render_template ("back_to_main.html")
        except Exception as e:
            print(e)
            print(data)
            return "error"
    return render_template('dish.html', dish=dish)

#送餐表單
@app.route('/send', methods=['GET', 'POST'])
def send1():
    global go_to_seat,status
    send = tablesend()  # 建立表單物件
    if send.validate_on_submit():  # 如果表單正常發送
        if (status=='kitchen'):   
            try:
                data = db.child('table').child(
                    f'table{send.table_no.data}').get().val()
                # data={
                # d1: ?
                # d2: ?
                # d3: ?
                # d4: ?
                # }
                data[f'd{send.seat_no.data}'] = send.status.data
                db.child('table').child(f'table{send.table_no.data}').update(data)
                go_to_seat = send.seat_no.data + send.table_no.data*10
                print('送餐到：',go_to_seat)
                return render_template ("back_to_clerk.html")
            except Exception as e:
                print(e)
                print(data)
                return "error"
        else:
            return render_template('sorry_clerk.html')
    return render_template('send.html', send=send)

# for customers
@app.route('/main', methods=['GET', 'POST'])
def main():
    global go_to_seat
    global status
    if request.method == 'POST':
        if request.form.get('action1') == '帶位':
            if (status=='standby'):
                i = 1  #桌號
                j = 1  #座號
                while 1:
                    seat = db.child('table').child(f'table{i}').child(f's{j}').get().val()
                    if seat ==False:
                        break
                    j+=1
                    if j==5:
                        j=1
                        i+=1
                        if i ==5:
                            go_to_seat = -1
                            return'sorry, no seat'
                go_to_seat = 10*i + j
                data = db.child('table').child(
                    f'table{i}').get().val()
                # data={
                # s1: ?
                # s2: ?
                # s3: ?
                # s4: ?
                # }
                data[f's{j}'] = True
                db.child('table').child(f'table{i}').update(data)
                return render_template ('finding.html')
            else:
                return render_template ('sorry.html')
        elif request.form.get('action3') == '完成':
            go_to_seat =0
            return render_template('main.html', form=form)
    elif request.method == 'GET':
        return render_template('main.html', form=form)

#for clerk    
@app.route('/clerk', methods=['GET', 'POST'])
def clerk():
    global go_to_seat,status
    if request.method == 'POST':
        if request.form.get('action_take') == '取餐':
            if (status=='standby'):
                go_to_seat = 52
                return render_template ('takefood_going.html')
            else:
                return render_template('sorry_clerk.html')
        if request.form.get('action_reset') == '重製':
            go_to_seat = 0
            status = "standby"
            return render_template('clerk.html')
    elif request.method == 'GET':
        return render_template('clerk.html', form=form)

class tabledit(FlaskForm):  # 定義表單物件
    table_no = IntegerField("請輸入桌號")
    seat_no = IntegerField("請輸入座位號碼")
    status = BooleanField('是否入座')
    submit = SubmitField("送出")

class tablesend(FlaskForm):  # 定義表單物件
    table_no = IntegerField("請輸入桌號")
    seat_no = IntegerField("請輸入座位號碼")
    status = SelectField("餐點狀態",choices = [('n','餐點完成')])
    submit = SubmitField("送出")

class tabledish(FlaskForm):  # 定義表單物件
    table_no = IntegerField("請輸入桌號",validators=[DataRequired()])
    seat_no = IntegerField("請輸入座位號碼",validators=[DataRequired()])
    status = SelectField("您的餐點",choices = [('a','漢堡'),('b','薯條')])
    submit = SubmitField("送出")

#和板子溝通
@app.route("/esp32", methods=['GET'])
def esp32():
    # return wanted data to esp32
    global go_to_seat,status
    if request.method == "GET":
        mode = request.args.get('mode')
        if mode == 'update': #esp32傳過來
            data_get  = str(request.args.get('hellofromesp'))
            if (data_get=='1'): # 
                status = 'takingfood'
            elif (data_get=='2'): #arrive kitchen
                status = 'kitchen'
            elif (data_get=='3'): #arrive kitchen
                status = 'running'
            elif (data_get=='4'): #arrive kitchen
                status = 'foodserving'
            elif (data_get=='5'): #arrive kitchen
                status = 'standby'
            elif (data_get=='6'): #arrive kitchen
                status = 'back'
            else:
                status =  'no good'
            print('status now:',status)
            return 'finished'
        elif mode == 'gather': #傳過去esp32
            which = request.args.get('which')
            if which == 'where_to_go':
                print('Go to:',go_to_seat)
                return str(go_to_seat)
            elif which == 'status':
                return status             

if __name__ == '__main__':
    # manager.run()
    # app.run(debug=True)
    app.run(host = '172.20.10.4')

