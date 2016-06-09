import web
from web import form

configpath="/home/amaurial/projetos/canpi/canpi.cfg"

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

    def getValue(self,key):
        return self.config[key]     





cm = configManager()
cm.readConfig()
apmode=True
gridenable=True

if cm.getValue("ap_mode")=="true":
    apmode=False
if cm.getValue("can_grid")=="true":
    apmode=False

myform = form.Form( 
    form.Checkbox('Apmode',checked=apmode), 
    form.Textbox("SSID",value=cm.getValue("ap_ssid")), 
    form.Textbox("Password", value=cm.getValue("ap_password")),
    form.Dropdown('Channel', ['1', '2', '3','4','5','6','7','8','9','10','11','12','13'],value=cm.getValue("ap_channel")), 
    form.Textbox("Router SSID",value=cm.getValue("router_ssid")), 
    form.Textbox("Router Password", value=cm.getValue("router_password")),    
    form.Textbox('Bonjour service name',value=cm.getValue("service_name")), 
    form.Textbox('ED Throttle Service port',value=cm.getValue("tcpport")),
    form.Checkbox('Cangrid enable',checked=gridenable),
    form.Textbox('Grid connect Service port',value=cm.getValue("cangrid_port")),
    form.Textbox('Logfile',value=cm.getValue("logfile"))) 

class index: 
    def GET(self): 
        form = myform()
        # make sure you create a copy of the form by calling it (line above)
        # Otherwise changes will appear globally
        return render.index(form,"Canpi Configuration")

    def POST(self): 
        form = myform() 
        #if not form.validates(): 
        return render.index(form,"Canpi Configuration")
        #else:
            # form.d.boe and form['boe'].value are equivalent ways of
            # extracting the validated arguments from the form.
        #    return "Grrreat success! boe: %s, bax: %s" % (form.d.Apmode, form['SSID'].value)



if __name__=="__main__":
    web.internalerror = web.debugerror
    app.run()
