import web
import os
import shutil
from web import form

#configpath="/home/amaurial/projetos/canpi/canpi.cfg"
configpath="/home/user/amauriala/Projects/canpi/canpi.cfg"

render = web.template.render('templates/')
urls = ('/', 'index')
app = web.application(urls, globals())

class configManager:
    config={}
    def readConfig(self):
	with open(configpath) as f:
	    for line in f:
               #print(line + "\n")
               if ((line.strip().find("#") >= 0) or (len(line.strip())<1)):
		   continue               
	       strval = line.strip().replace('"','')               
	       (key, val) = strval.split('=')
	       self.config[key.strip()] = val
        f.close()

    def getValue(self,key):
        return self.config[key] 
    
    def setValue(self,key,value):
        self.config[key] = value
   
    def saveFile(self):
        #create a copy of the file
        tmpfile = configpath + ".tmp"
        with open(tmpfile,'w+') as f:
            for k,v in self.config.iteritems():
                line = ""
                #dont put quotes on numbers
                if k in ["tcpport","cangrid_port","ap_channel","node_number"]:
                     line = k + "=" + v + "\n"
                else:
                     line = k + "=\"" + v + "\"\n" 
                f.write(line)
        f.close()
        #move the file
        if os.path.exists(tmpfile):
           shutil.move(tmpfile,configpath)

cm = configManager()
cm.readConfig()
apmode=True
gridenable=True

if cm.getValue("ap_mode")!="true":
    apmode=False
if cm.getValue("can_grid")!="true":
    gridenable=False

myform = form.Form( 
    form.Checkbox('Apmode',checked=apmode,value="apmode",id="tapmode"), 
    form.Textbox("SSID",value=cm.getValue("ap_ssid"),id="apssid"), 
    form.Textbox("Password", value=cm.getValue("ap_password"),id="appasswd"),
    form.Dropdown('Channel', ['1', '2', '3','4','5','6','7','8','9','10','11','12','13'],value=cm.getValue("ap_channel"),id="channel"), 
    form.Textbox("Router SSID",value=cm.getValue("router_ssid"),id="routerssid"), 
    form.Textbox("Router Password", value=cm.getValue("router_password"),id="routerpasswd"),    
    form.Textbox('Bonjour service name',value=cm.getValue("service_name"),id="servicename"), 
    form.Textbox('ED Throttle Service port',value=cm.getValue("tcpport"),id="tcpport"),
    form.Checkbox('Cangrid enable',checked=gridenable,value="gridenable",id="cangrid"),
    form.Textbox('Grid connect Service port',value=cm.getValue("cangrid_port"),id="cangripport"),
    form.Textbox('Logfile',value=cm.getValue("logfile"),id="logfile"),
    form.Button("btn", id="btnSave", value="save", html="Save changes"),
    form.Button("btn", id="btnRestart", value="restart", html="Reboot")) 

class index: 
    def GET(self): 
        form = myform()
        # make sure you create a copy of the form by calling it (line above)
        # Otherwise changes will appear globally
        return render.index(form,"Canpi Configuration")

    def POST(self): 
        form = myform() 
        if not form.validates(): 
		return render.index(form,"Canpi Configuration")
        userData = web.input()
        if userData.btn == "save":        
		#get all the values and update
		cm.setValue("ap_mode",str(form['Apmode'].checked))
		cm.setValue("ap_ssid",str(form['SSID'].value))
		cm.setValue("ap_password",str(form['Password'].value))
		cm.setValue("ap_channel",str(form['Channel'].value))
		cm.setValue("router_ssid",str(form['Router SSID'].value))        
		cm.setValue("router_password",str(form['Router Password'].value))
		cm.setValue("can_grid",str(form['Cangrid enable'].checked))
		cm.setValue("cangrid_port",str(form['Grid connect Service port'].value))
                cm.setValue("service_name",str(form['Bonjour service name'].value))
		cm.setValue("tcpport",str(form['ED Throttle Service port'].value))
		cm.setValue("logfile",str(form['Logfile'].value))
		cm.saveFile()
        	return render.index(form,"Canpi Configuration Saved")
        if userData.btn == "restart": 
                return render.index(form,"Restarting ...")

        return render.index(form,"Canpi Configuration")
        #else:
            # form.d.boe and form['boe'].value are equivalent ways of
            # extracting the validated arguments from the form.
        #    return "Grrreat success! boe: %s, bax: %s" % (form.d.Apmode, form['SSID'].value)

if __name__=="__main__":
    web.internalerror = web.debugerror
    app.run()
