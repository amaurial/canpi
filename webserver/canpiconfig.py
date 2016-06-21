import web
import os
import subprocess
import shutil
from web import form
import time
import shlex
from subprocess import Popen, PIPE

#configpath="/home/amaurial/projetos/canpi/canpi.cfg"
#configpath="/home/user/amauriala/Projects/canpi/canpi.cfg"
configpath="/home/pi/canpi/canpi.cfg"

render = web.template.render('templates/')
urls = ('/', 'index')
app = web.application(urls, globals(),autoreload=True)

class configManager:
    config={}
    def readConfig(self):
        self.config.clear()
        with open(configpath) as f:
            for line in f:
                #print(line + "\n")
                if ((line.strip().find("#") >= 0) or (len(line.strip())<1)):
                    continue
                strval = line.strip().replace('"','')
                strval = strval.strip().replace(';','')
                strval = strval.strip().replace(' = ','=')
                (key, val) = strval.split('=')
                self.config[key.strip()] = val
            f.close()
            #print("\n########## CONFIG ##########\n")
            #print(self.config)

    def getValue(self,key):
        return self.config[key]

    def setValue(self,key,value):
        self.config[key] = value

    def saveFile(self):
        #create a copy of the file
        print("Saving file")
        tmpfile = configpath + ".tmp"
        with open(tmpfile,'w+') as f:
            for k,v in self.config.iteritems():
                line = ""
                #dont put quotes on numbers
                if k in ["tcpport","cangrid_port","ap_channel","node_number","canid"]:
                     line = k + "=" + v + "\n"
                else:
                     line = k + "=\"" + v + "\"\n"
                #print(line)
                f.write(line)
        f.close()
        #move the file
        if os.path.exists(tmpfile):
           print("Moving the file")
           shutil.move(tmpfile,configpath)

cm = configManager()
cm.readConfig()
apmode=True
gridenable=True

if cm.getValue("ap_mode").lower()!="true":
    apmode=False
if cm.getValue("can_grid").lower()!="true":
    gridenable=False

id_apmode="Apmode"
id_ssid="SSID"
id_password="Password"
id_channel="Channel"
id_router_ssid="Router SSID"
id_router_password="Router Password"
id_bonjour_name="Bonjour service name"
id_ed_tcpport="ED Throttle Service port"
id_grid_enable="Cangrid enable"
id_grid_port="Grid connect Service port"
id_logfile="Logfile"
id_loglevel="Log Level"
id_canid="Can ID"
id_turnout_file="Turnouts"

myform = form.Form(
    form.Checkbox(id_apmode,checked=apmode,value="apmode",id="tapmode"),
    form.Textbox(id_ssid,value=cm.getValue("ap_ssid"),id="apssid"),
    form.Textbox(id_password, value=cm.getValue("ap_password"),id="appasswd"),
    form.Dropdown(id_channel, ['1', '2', '3','4','5','6','7','8','9','10','11','12','13'],value=cm.getValue("ap_channel")),
    form.Textbox(id_router_ssid,value=cm.getValue("router_ssid"),id="routerssid"),
    form.Textbox(id_router_password, value=cm.getValue("router_password"),id="routerpasswd"),
    form.Textbox(id_bonjour_name,value=cm.getValue("service_name"),id="servicename"),
    form.Textbox(id_ed_tcpport,value=cm.getValue("tcpport"),id="tcpport"),
    form.Checkbox(id_grid_enable,checked=gridenable,value="gridenable",id="cangrid"),
    form.Textbox(id_grid_port,value=cm.getValue("cangrid_port"),id="cangripport"),
    form.Textbox(id_canid,value=cm.getValue("canid")),
    form.Textbox(id_turnout_file,value=cm.getValue("turnout_file")),
    form.Dropdown(id_loglevel, ['INFO', 'WARN', 'DEBUG'],value=cm.getValue("loglevel")),
    #form.Textbox(id_logfile,value=cm.getValue("logfile"),id="logfile"),
    form.Button("btn", id="btnSave", value="save", html="Save changes"),
    form.Button("btn", id="btnApply", value="apply", html="Apply changes and reboot"),
    form.Button("btn", id="btnRestart", value="restart", html="Restart Throttle service"))
def reloadMyForm():
    global myform
    global apmode
    global gridenable

    apmode=True
    gridenable=True
    if cm.getValue("ap_mode").lower()!="true":
        apmode=False
    if cm.getValue("can_grid").lower()!="true":
        gridenable=False

    myform = form.Form(
        form.Checkbox(id_apmode,checked=apmode,value="apmode",id="tapmode"),
        form.Textbox(id_ssid,value=cm.getValue("ap_ssid"),id="apssid"),
        form.Textbox(id_password, value=cm.getValue("ap_password"),id="appasswd"),
        form.Dropdown(id_channel, ['1', '2', '3','4','5','6','7','8','9','10','11','12','13'],value=cm.getValue("ap_channel")),
        form.Textbox(id_router_ssid,value=cm.getValue("router_ssid"),id="routerssid"),
        form.Textbox(id_router_password, value=cm.getValue("router_password"),id="routerpasswd"),
        form.Textbox(id_bonjour_name,value=cm.getValue("service_name"),id="servicename"),
        form.Textbox(id_ed_tcpport,value=cm.getValue("tcpport"),id="tcpport"),
        form.Checkbox(id_grid_enable,checked=gridenable,value="gridenable",id="cangrid"),
        form.Textbox(id_grid_port,value=cm.getValue("cangrid_port"),id="cangripport"),
        form.Textbox(id_canid,value=cm.getValue("canid")),
        form.Textbox(id_turnout_file,value=cm.getValue("turnout_file")),
        form.Dropdown(id_loglevel, ['INFO', 'WARN', 'DEBUG'],value=cm.getValue("loglevel")),
        #form.Textbox(id_logfile,value=cm.getValue("logfile"),id="logfile"),
        form.Button("btn", id="btnSave", value="save", html="Save changes"),
        form.Button("btn", id="btnApply", value="apply", html="Apply changes and reboot"),
        form.Button("btn", id="btnRestart", value="restart", html="Restart Throttle service"))

def restartPi():
    #time.sleep(3)
    os.system("/etc/init.d/start_canpi.sh configure")

class index:

    def GET(self):
        cm.readConfig()
        reloadMyForm()

        form = myform()
        # make sure you create a copy of the form by calling it (line above)
        # Otherwise changes will appear globally
        return render.index(form,"Canpi Configuration","")

    def POST(self):
        userData = web.input()
        ctx=web.ctx
        myuri = ctx.realhome
        form = myform()
        form.fill()
        #if not form.validates():
        #    return render.index(form,"Canpi Configuration")
        print ("Button " + userData.btn)
        if userData.btn == "save":
            #get all the values and update
            cm.setValue("ap_mode",str(form[id_apmode].checked))
            cm.setValue("ap_ssid",str(form[id_ssid].value))
            cm.setValue("ap_password",str(form[id_password].value))
            cm.setValue("ap_channel",str(form[id_channel].value))
            cm.setValue("router_ssid",str(form[id_router_ssid].value))
            cm.setValue("router_password",str(form[id_router_password].value))
            cm.setValue("can_grid",str(form[id_grid_enable].checked))
            cm.setValue("cangrid_port",str(form[id_grid_port].value))
            cm.setValue("service_name",str(form[id_bonjour_name].value))
            cm.setValue("tcpport",str(form[id_ed_tcpport].value))
            #cm.setValue("logfile",str(form[id_logfile].value))
            cm.setValue("loglevel",str(form[id_loglevel].value))
            cm.setValue("canid",str(form[id_canid].value))
            cm.setValue("turnout_file",str(form[id_turnout_file].value))
            cm.saveFile()
            raise web.seeother('/')
        if userData.btn == "apply":
            print("Apply button pressed")
            #subprocess.call(['sudo /etc/init.d/start_canpi.sh', 'configure'])
            cpid=os.fork()
            if cpid==0:
                restartPi()
            #os.system("/etc/init.d/start_canpi.sh configure")
            return render.reboot("Restarting",myuri)
        if userData.btn == "restart":
            print("Restart button pressed")
            #render.restart("Restarting",myuri)
            os.system("/etc/init.d/start_canpi.sh restartcanpi")
            #subprocess.call( [ 'sudo /bin/bash /etc/init.d/start_canpi.sh', 'restartcanpi'])
            exitcode,out,err = self.get_exitcode_stdout_stderr("ps -ef")
            msg=""
            if "/home/pi/canpi/canpi" in out:
                msg="Service restarted"
            else:
                msg="Restart failed"
            #render.index(form,"Restarting PI ...")
            #subprocess.call( [ 'sudo /bin/bash /etc/init.d/start_canpi.sh', 'restart'])
            return render.index(form,"Canpi Configuration",msg)

        return render.index(form,"Canpi Configuration")
    def get_exitcode_stdout_stderr(self,cmd):
        """
        Execute the external command and get its exitcode, stdout and stderr.
        """
        args = shlex.split(cmd)

        proc = Popen(args, stdout=PIPE, stderr=PIPE)
        out, err = proc.communicate()
        exitcode = proc.returncode
        #
        return exitcode, out, err



if __name__=="__main__":
    web.internalerror = web.debugerror
    web.config.debug = True
    app.run()
