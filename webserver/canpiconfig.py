import web
import os
import subprocess
import shutil
from web import form
import time
import shlex
from subprocess import Popen, PIPE

#configpath="/home/amaurial/projetos/canpi/canpi.cfg"
configpath="/home/user/amauriala/Projects/canpi/canpi.cfg"
#configpath="/home/pi/canpi/canpi.cfg"

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
        if key in self.config:
            return self.config[key]
        return ""

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
                if k in ["tcpport","cangrid_port","ap_channel","node_number","canid","button_pin","green_led_pin","yellow_led_pin"]:
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

id_apmode="apmode"
id_ssid="ap_ssid"
id_password="ap_password"
id_channel="ap_channel"
id_router_ssid="router_ssid"
id_router_password="router_password"
id_bonjour_name="service_name"
id_ed_tcpport="tcpport"
id_grid_enable="gridenable"
id_grid_port="cangrid_port"
id_logfile="logfile"
id_loglevel="loglevel"
id_canid="canid"
id_turnout_file="turnout_file"
id_fns_momentary="fn_momentary"
id_btn_save="btnSave"
id_btn_apply="btnApply"
id_btn_restart="btnRestart"

desc_apmode="Apmode"
desc_ssid="SSID"
desc_password="Password"
desc_channel="Channel"
desc_router_ssid="Router SSID"
desc_router_password="Router Password"
desc_bonjour_name="Bonjour service name"
desc_ed_tcpport="ED Throttle Service port"
desc_grid_enable="Cangrid enable"
desc_grid_port="Grid connect Service port"
desc_logfile="Logfile"
desc_loglevel="Log Level"
desc_canid="Can ID"
desc_turnout_file="Turnouts"
desc_fns_momentary="Fns momentary"
desc_btn_save="btnSave"
desc_btn_apply="btnApply"
desc_btn_restart="btnRestart"


myform = form.Form(
    form.Checkbox(id_apmode,description=desc_apmode,checked=apmode,value="apmode",id="tapmode"),
    form.Textbox(id_ssid,description=desc_ssid,value=cm.getValue("ap_ssid"),id="apssid"),
    form.Textbox(id_password,description=desc_password, value=cm.getValue("ap_password"),id="appasswd"),
    form.Dropdown(id_channel, ['1', '2', '3','4','5','6','7','8','9','10','11','12','13'],value=cm.getValue("ap_channel")),
    form.Textbox(id_router_ssid,description=desc_router_ssid,value=cm.getValue("router_ssid"),id="routerssid"),
    form.Textbox(id_router_password, description=desc_router_password,value=cm.getValue("router_password"),id="routerpasswd"),
    form.Textbox(id_bonjour_name,description=desc_bonjour_name,value=cm.getValue("service_name"),id="servicename"),
    form.Textbox(id_ed_tcpport,description=desc_ed_tcpport,value=cm.getValue("tcpport"),id="tcpport"),
    form.Checkbox(id_grid_enable,description=desc_grid_enable,checked=gridenable,value="gridenable",id="cangrid"),
    form.Textbox(id_grid_port,description=desc_grid_port,value=cm.getValue("cangrid_port"),id="cangripport"),
    form.Textbox(id_canid,description=desc_canid,value=cm.getValue("canid")),
    form.Textbox(id_fns_momentary,description=desc_fns_momentary,value=cm.getValue("fn_momentary")),
    form.Textbox(id_turnout_file,description=desc_turnout_file,value=cm.getValue("turnout_file")),
    form.Dropdown(id_loglevel,  ['INFO', 'WARN', 'DEBUG'],value=cm.getValue("loglevel")),
    #form.Textbox(id_logfile,description=,value=cm.getValue("logfile"),id="logfile"),
    form.Button("btn", id=id_btn_save, value="save", html="Save changes"),
    form.Button("btn", id=id_btn_apply, value="apply", html="Apply changes and reboot"),
    form.Button("btn", id=id_btn_restart, value="restart", html="Restart Throttle service"))
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
        form.Checkbox(id_apmode,description=desc_apmode,checked=apmode,value="apmode",id="tapmode"),
        form.Textbox(id_ssid,description=desc_ssid,value=cm.getValue("ap_ssid"),id="apssid"),
        form.Textbox(id_password,description=desc_password, value=cm.getValue("ap_password"),id="appasswd"),
        form.Dropdown(id_channel, ['1', '2', '3','4','5','6','7','8','9','10','11','12','13'],value=cm.getValue("ap_channel")),
        form.Textbox(id_router_ssid,description=desc_router_ssid,value=cm.getValue("router_ssid"),id="routerssid"),
        form.Textbox(id_router_password, description=desc_router_password,value=cm.getValue("router_password"),id="routerpasswd"),
        form.Textbox(id_bonjour_name,description=desc_bonjour_name,value=cm.getValue("service_name"),id="servicename"),
        form.Textbox(id_ed_tcpport,description=desc_ed_tcpport,value=cm.getValue("tcpport"),id="tcpport"),
        form.Checkbox(id_grid_enable,description=desc_grid_enable,checked=gridenable,value="gridenable",id="cangrid"),
        form.Textbox(id_grid_port,description=desc_grid_port,value=cm.getValue("cangrid_port"),id="cangripport"),
        form.Textbox(id_canid,description=desc_canid,value=cm.getValue("canid")),
        form.Textbox(id_fns_momentary,description=desc_fns_momentary,value=cm.getValue("fn_momentary")),
        form.Textbox(id_turnout_file,description=desc_turnout_file,value=cm.getValue("turnout_file")),
        form.Dropdown(id_loglevel,  ['INFO', 'WARN', 'DEBUG'],value=cm.getValue("loglevel")),
        #form.Textbox(id_logfile,description=,value=cm.getValue("logfile"),id="logfile"),
        form.Button(id_btn_save, id=id_btn_save, value="save", html="Save changes"),
        form.Button(id_btn_apply, id=id_btn_apply, value="apply", html="Apply changes and reboot"),
        form.Button(id_btn_restart, id=id_btn_restart, value="restart", html="Restart Throttle service"))

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

        if id_btn_save in userData:
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
            cm.setValue("fn_momentary",str(form[id_fns_momentary].value))
            cm.setValue("turnout_file",str(form[id_turnout_file].value))
            cm.saveFile()
            raise web.seeother('/')
        if id_btn_apply in userData:
            print("Apply button pressed")
            #subprocess.call(['sudo /etc/init.d/start_canpi.sh', 'configure'])
            cpid=os.fork()
            if cpid==0:
                restartPi()
            #os.system("/etc/init.d/start_canpi.sh configure")
            return render.reboot("Restarting",myuri)
        if id_btn_restart in userData:
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
