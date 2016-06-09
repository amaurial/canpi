import web
from web import form

configpath="/home/user/amauriala/Projects/canpi/canpi.cfg"

render = web.template.render('templates/')

class configManager:
    config={}
    def readConfig(self):
	with open(configpath) as f:
	    for line in f:
               #print(line + "\n")
               if ((line.strip().find("#") >= 0) or (len(line.strip())<1)):
		   continue
               
	       strval = line.strip()
	       (key, val) = strval.split('=')
	       self.config[key.strip()] = val

    def getValue(self,key):
        return self.config[key]     



urls = ('/', 'index')
app = web.application(urls, globals())

cm = configManager()
cm.readConfig()

myform = form.Form( 
    form.Checkbox('Apmode'), 
    form.Textbox("SSID",value=cm.getValue("ap_ssid")), 
    form.Textbox("Password", value=cm.getValue("ap_password")), 
    form.Textbox('Channel',value=cm.getValue("ap_channel")), 
    form.Textbox('Bonjour service name',value=cm.getValue("service_name")), 
    form.Dropdown('Channel', ['mustard', 'fries', 'wine'])) 

class index: 
    def GET(self): 
        form = myform()
        # make sure you create a copy of the form by calling it (line above)
        # Otherwise changes will appear globally
        return render.formtest(form)

    def POST(self): 
        form = myform() 
        if not form.validates(): 
            return render.formtest(form)
        else:
            # form.d.boe and form['boe'].value are equivalent ways of
            # extracting the validated arguments from the form.
            return "Grrreat success! boe: %s, bax: %s" % (form.d.Apmode, form['bax'].value)



if __name__=="__main__":
    web.internalerror = web.debugerror
    app.run()
