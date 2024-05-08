import json
from flask import Flask
from flask_restful import Api, Resource, reqparse
import pyrebase
import firebase_admin
from firebase_admin import credentials
from firebase_admin import firestore

# Use the application default credentials
cred = credentials.Certificate(
    'C:/Users/EMILY/Desktop/Project/Server/key.json')
firebase_admin.initialize_app(cred)

config = {
    "apiKey": "apiKey",
    "authDomain": "dormDB.firebaseapp.com",
    "databaseURL": "https://dormdb-622b7-default-rtdb.asia-southeast1.firebasedatabase.app/",
    "storageBucket": "dormDB.appspot.com",
    "serviceAccount": "C:/Users/EMILY/Desktop/Project/Server/key.json"
}
firebase = pyrebase.initialize_app(config)
Rdb = firebase.database()
db = firestore.client()
app = Flask(__name__)
api = Api(app)


std_parser = reqparse.RequestParser()
std_parser.add_argument("email", type=str)
std_parser.add_argument("name", type=str)
std_parser.add_argument("phone", type=str)
std_parser.add_argument("room_bed", type=str)
std_parser.add_argument("package", type=dict, action="append")
std_parser.add_argument("id", type=str)

repair_parser = reqparse.RequestParser()
repair_parser.add_argument("type",type=str)
repair_parser.add_argument("no",type=str)
repair_parser.add_argument("depict",type=str)
repair_parser.add_argument("index",type=int)
repair_parser.add_argument("con",type=str)

studentList = [
    {
        "sname": "Millie",
        "rddom-bed": "3409-4"
    }, {
        "sname": "Chichi",
        "room-bed": "3631-3"
    }, {
        "sname": "George",
        "room-bed": "2145-2"
    }
]


class StudentList(Resource):
    def get(self):
        student_ref = db.collection(u'student')
        docs = student_ref.stream()
        s={}
        for doc in docs:
            print(f'{doc.id} => {doc.to_dict()}')
            s[doc.id]=doc.to_dict() 
        return s

    def post(self):
        new_student=std_parser.parse_args()
        res = {
            "msg":"新增使用者",
            "新增使用者":new_student
        }
        data = {
            "email": new_student["email"],
            "name": new_student["name"],
            "phone": new_student["phone"],
            "room_bed": new_student["room_bed"],
            "package": new_student["package"],
        }
        db.collection(u'student').document(new_student["id"]).set(data)
        return res

    def put(self):
        new_student=std_parser.parse_args()
        try:
            res = {
                "msg":"更新使用者",
                "更新使用者":new_student
            }
            db.collection(u'student').document(new_student["id"]).update(new_student)
        except:
            res = {
                "msg":"查無此學號",
                "id":new_student["id"]
            }
        return res
        
    def delete(self):
        db.collection(u'student').document(u'4012345687').delete()
        print('資料刪除成功')
        return 1

    def delete_collection(self):
        doc_ref = db.collection('test')
        docs = doc_ref.get()
        # Delete Collection
        for delete_list in docs:
            doc_del = db.collection('test').document(delete_list.id)
            doc_del.delete()
            print('Delete Doc: {} Complete.'.format(delete_list.id))
        print('集合刪除成功')
        return 1

class AdminList(Resource):
    def get(self):
        admin_ref = db.collection(u'admin')
        docs = admin_ref.stream()
        a = {}
        for doc in docs:
            print(f'{doc.id} => {doc.to_dict()}')
            a[doc.id] = doc.to_dict()
        return a

    def post(self):
        data = {
            u'email': u'banana@gms.tku.edu.tw',
            u'name': u'BB',
            u'phone': '0900369852'
        }
        db.collection(u'admin').document(u'45678').set(data)
        print(f'資料新增成功')
        return data

    def put(self):
        data = {
            'name': 'YAYA'
        }
        doc_ref = db.collection('admin').document('34567')
        doc_ref.update(data)
        print('資料更新成功')
        return data

    def delete(self):
        db.collection(u'test').document(u'3596H8cze6Qo6B0pRKj9').delete()
        print('資料刪除成功')
        return 1

    def delete_collection(self):
        doc_ref = db.collection('test')
        docs = doc_ref.get()
        # Delete Collection
        for delete_list in docs:
            doc_del = db.collection('test').document(delete_list.id)
            doc_del.delete()
            print('Delete Doc: {} Complete.'.format(delete_list.id))
        print('集合刪除成功')
        return 1


class Student(Resource):
    def get(self, sid):
        try:
            res = studentList[sid]
        except IndexError:
            res = {
                "error msg": f'查無sid為{sid}資料'
            }
        return res

class RepairList(Resource):
    def get(self,type):
        data = Rdb.child(type).get().val()
        res=[]
        for i in data.keys():
            for p,j in enumerate(data[i]):
                if(j["rep"]!=""):
                    res.append({"location" : f'{type}_{i}_{p}',
                                "machine" : j
                                })
        return res

class Repair(Resource):
    def post(self): #學生用
        try:
            input_data=repair_parser.parse_args()
            subkey=input_data["no"].split('_')
            Rdb.child(input_data["type"]).child(subkey[0]).child(int(subkey[1])-1).update({"rep":input_data["depict"]})
            res = {
                "msg":"資料更新成功"
            }
        except:
            res = {
                "msg":"資料更新失敗"
            }
        return res
    def put(self): #管理員用
        try:
            input_data=repair_parser.parse_args()
            R=RepairList.get(self,input_data["type"])
            location=R[input_data["index"]]["location"].split('_')
            if(input_data["con"]=="usable"):
                update_data={"con" : "usable","rep" : ""}
            elif (input_data["con"]=="broken"):
                update_data={"con" : "broken"}
            Rdb.child(location[0]).child(location[1]).child(location[2]).update(update_data)
            res = {
                "msg":"管理員報修成功"
            }
        except:
            res = {
                "msg":"管理員報修失敗"
            }
        return res
    
    

        

api.add_resource(StudentList, '/student')
api.add_resource(AdminList, '/admin')
api.add_resource(Student, '/student/<int:sid>')
api.add_resource(RepairList, '/repair/<string:type>')
api.add_resource(Repair, '/repair')


if __name__ == '__main__':
    app.run(debug=True)
