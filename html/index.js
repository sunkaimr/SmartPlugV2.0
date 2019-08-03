
var LocalTime = 0;
var relaystatus = false;
var SysData;
var DelayData;
var DelayRefreshTime="";


$(document).ready(function () {

	TimerClick();
	getDate();
	getDevName();
	refreshRelay();
	setInterval(DateDisplay, 1000);

	$("#relay").click(setRelay);
	$("#date").click(getDate);
	$("#timer").click(TimerClick);
	$("#delay").click(DelayClick);
	$("#set").click(SetClick);
	$("#setCommit").click(setCommitClick);
	$("#chooseBin").click(chooseBinClick);
	$("#scanWifi").click(scanWifiClick);
	$("#upload").click(uploadClick);
	$("#reboot").click(rebootClick);
	$("#reset").click(resetClick);
	$("#modeSelect").change(modeChange);
    $("#PlatformSelect").change(PlatformSelectChange);
	$("#file").change(binFileChange);
	$("#information").click(getInfomation);
	$("#rebootModalButton").click(reboot);
	$("#resetModalButton").click(reset);

	$("#content").click(function () {
		$("#navBar").collapse('hide');
	});

	$(function () {
		$('#timerSubmitForm').bootstrapValidator({
			message: 'This value is not valid',
			feedbackIcons: {
				valid: 'glyphicon glyphicon-ok',
				invalid: 'glyphicon glyphicon-remove',
				//validating: 'glyphicon glyphicon-refresh'
			},
			fields: {
				timeName: {
					message: 'Timer name is not valid',
					validators: {
						notEmpty: {
							message: '名称不能为空'
						},
						stringLength: {
							min: 1,
							max: 32,
							message: '长度必须小于32（中文占3个字符）'
						}
					}
				},
				onTime: {
					message: 'Off Time is not valid',
					validators: {
						notEmpty: {
							message: '开启时间不能为空'
						},
					}
				},
				offTime: {
					message: 'Off Time is not valid',
					validators: {
						notEmpty: {
							message: '关闭时间不能为空'
						},
					}
				},
				timerCascodeNum: {
					validators: {
						notEmpty: {
							message: '关联延时不能为空'
						},
						greaterThan: {
							value: 0,
							message: '不能小于1',
						},
						lessThan: {
							value: 10,
							message: '不能大于10'
						}
					}
				}
			}
		}).on('success.form.bv', function (e) {//点击提交之后
			e.preventDefault();
			var $form = $(e.target);
			//var bv = $form.data('bootstrapValidator');
			var jsonDataAry=[];
			var jsonData = {};

			var numStr=$('#timerModalHead').text().trim().split(/\s+/);
			jsonData.Num = parseInt(numStr[numStr.length-1]);
			jsonData.Name = document.getElementById("timeName").value;
			jsonData.Enable = document.getElementById("timeEnable").checked;
			jsonData.OnEnable = document.getElementById("onTimeEnable").checked;
			jsonData.OffEnable = document.getElementById("offTimeEnable").checked;
			jsonData.Cascode = document.getElementById("timerCascodeEnable").checked;
			jsonData.CascodeNum = parseInt(document.getElementById("timerCascodeNum").value);
			jsonData.Week = stringConversionWeek();
			jsonData.OnTime = document.getElementById("onTime").value;
			jsonData.OffTime = document.getElementById("offTime").value;
			jsonDataAry.push(jsonData);

			$.ajax({
				url: $form.attr('action'),
				type: "POST",
				contentType: "application/json",
				data:JSON.stringify(jsonDataAry),
				success: function () {
					//alert("成功");
				}
			});
			$("#timerSubmitModal").modal("toggle")
			document.getElementById("timerSubmitBtn").disabled=false;
			TimerClick();
		});
	});

	$(function () {
		$('#delaySubmitForm').bootstrapValidator({
			message: 'This value is not valid',
			feedbackIcons: {
				valid: 'glyphicon glyphicon-ok',
				invalid: 'glyphicon glyphicon-remove',
				//validating: 'glyphicon glyphicon-refresh'
			},
			fields: {
				delayName:{
					message: 'Timer name is not valid',
						validators: {
						notEmpty: {
							message: '名称不能为空'
						},
						stringLength: {
							min: 1,
							max: 32,
							message: '长度必须小于32（中文占3个字符）'
						}
					}
				},
				onInterval: {
					message: 'Off Time is not valid',
					validators: {
						notEmpty: {
							message: '开启间隔不能为空'
						},
					}
				},
				offInterval: {
					message: 'Off Time is not valid',
					validators: {
						notEmpty: {
							message: '关闭间隔不能为空'
						},
					}
				},
				timerCascodeNum: {
					validators: {
						notEmpty: {
							message: '关联延时不能为空'
						},
						greaterThan: {
							value: 0,
							message: '不能小于1',
						},
						lessThan: {
							value: 10,
							message: '不能大于10'
						}
					}
				},
				cycleTimes: {
					validators: {
						notEmpty: {
							message: '重复次数不能为空'
						},
						greaterThan: {
							value: 0,
							message: '不能小于0',
						},
						lessThan: {
							value: 10,
							message: '不能大于9999'
						}
					}
				}
			}
		}).on('success.form.bv', function (e) {//点击提交之后
			e.preventDefault();
			var $form = $(e.target);
			var jsonDataAry=[];
			var jsonData = {};

			var numStr=$('#delayModalHead').text().trim().split(/\s+/);
			jsonData.Num = parseInt(numStr[numStr.length-1]);
			jsonData.Name = document.getElementById("delayName").value;
			jsonData.Enable = document.getElementById("delayEnable").checked;
			jsonData.OnEnable = document.getElementById("onIntervalEnable").checked;
			jsonData.OffEnable = document.getElementById("onIntervalEnable").checked;
			jsonData.Cascode = document.getElementById("delayCascodeEnable").checked;
			jsonData.CascodeNum = parseInt(document.getElementById("delayCascodeNum").value);
			jsonData.CycleTimes = parseInt(document.getElementById("cycleTimes").value);
			jsonData.OnInterval = document.getElementById("onInterval").value;
			jsonData.OffInterval = document.getElementById("offInterval").value;
			jsonDataAry.push(jsonData);

			$.ajax({
				url: $form.attr('action'),
				type: "POST",
				contentType: "application/json",
				data:JSON.stringify(jsonDataAry),
				success: function () {
					//alert("成功");
				},
				error: function (jqXHR, textStatus, errorThrown) {
					ShowInfo(jqXHR.responseText);
				}
			});
			DelayClick();
			$("#delaySubmitModal").modal("toggle")
			document.getElementById("delaySubmitBtn").disabled=false;
		});
	});

});

function binFileChange() {
	var path=$('#file').val().split("\\");
	var fileName = path[path.length-1];
	$('#binFile').val(fileName)
}

function modeChange(){
	if( $("#modeSelect").val() == 1 ){
		if (SysData.WifiMode == 2){
            $("#wifiCustomClass").removeClass("hidden");
            $("#wifiPasswdClass").removeClass("hidden");
        }else{
            $("#wifiClass").removeClass("hidden");
            $("#wifiPasswdClass").removeClass("hidden");
        }
	}else{
		$("#wifiClass").addClass("hidden");
		$("#wifiPasswdClass").addClass("hidden");
        $("#wifiCustomClass").addClass("hidden");
	}
    PlatformSelectChange();
}

function PlatformSelectChange(){

    if ($("#modeSelect").val() == 2){
        $("#PlatformSelectClass").addClass("hidden");
        $("#ProductKeyClass").addClass("hidden");
        $("#DeviceNameClass").addClass("hidden");
        $("#DeviceSecretClass").addClass("hidden");
        $("#BigiotDeviceIdClass").addClass("hidden");
        $("#BigiotApiKeyClass").addClass("hidden");
    }else{
        $("#PlatformSelectClass").removeClass("hidden");

        if( $("#PlatformSelect").val() == 1 ) {
            $("#ProductKeyClass").removeClass("hidden");
            $("#DeviceNameClass").removeClass("hidden");
            $("#DeviceSecretClass").removeClass("hidden");

            $("#BigiotDeviceIdClass").addClass("hidden");
            $("#BigiotApiKeyClass").addClass("hidden");
        }else if (  $("#PlatformSelect").val() == 2 ){
            $("#ProductKeyClass").addClass("hidden");
            $("#DeviceNameClass").addClass("hidden");
            $("#DeviceSecretClass").addClass("hidden");

            $("#BigiotDeviceIdClass").removeClass("hidden");
            $("#BigiotApiKeyClass").removeClass("hidden");
        }else{
            $("#ProductKeyClass").addClass("hidden");
            $("#DeviceNameClass").addClass("hidden");
            $("#DeviceSecretClass").addClass("hidden");

            $("#BigiotDeviceIdClass").addClass("hidden");
            $("#BigiotApiKeyClass").addClass("hidden");
        }

    }
}

function SetFormat( s ){
	if ( s < 10 )
		s = "0"+s;
	return s;
}

function FormatDate(nS){
	var week=new Array();
	week[0]="日"; week[1]="一"; week[2]="二"; week[3]="三"; week[4]="四"; week[5]="五"; week[6]="六";
	var today = new Date(parseInt(nS) * 1000);
	var y=SetFormat(today.getFullYear());
	var M=SetFormat(today.getMonth()+1);
	var d=SetFormat(today.getDate());
	var h=SetFormat(today.getHours());
	var m=SetFormat(today.getMinutes());
	var s=SetFormat(today.getSeconds());
	var w=today.getDay();
	return y + "-" + M + "-" + d + " " + h + ":" + m + ":" + s + " 星期" + week[w] ;
}

function DateDisplay(){
	LocalTime++;
	$("#date").text(FormatDate(LocalTime));
	if (parseInt(LocalTime%60) == 1){
		refreshRelay();
	}

	if(parseInt(LocalTime%60) == 3){
        var today = new Date(parseInt(LocalTime) * 1000);
        var str = SetFormat(today.getHours())+":"+SetFormat(today.getMinutes());
        if ( str == DelayRefreshTime ){
            DelayClick();
        }
    }
}

function getDate(){
	$.get("/date",function(data, status){
		if (status == "success"){
			if ( data.SyncTime ){
				dateStr=(data.Date).replace(/\-/g, "/");
				LocalTime = Date.parse(dateStr)/1000;
				$("#date").text(FormatDate(LocalTime));
			}else {
				var today = new Date();
				var y=SetFormat(today.getFullYear());
				var M=SetFormat(today.getMonth()+1);
				var d=SetFormat(today.getDate());
				var h=SetFormat(today.getHours());
				var m=SetFormat(today.getMinutes());
				var s=SetFormat(today.getSeconds());
				dateStr = y+"-"+M+"-"+d+" "+h+":"+m+":"+s;
				dataJson = "{\"Date\": \""+dateStr+"\"}";
				$.ajax({
					type: "POST",
					url: "/date",
					contentType: "application/json",
					data: dataJson,
					success: function (data) {
						LocalTime = Date.parse(dateStr)/1000;
					  },
					error: function (jqXHR, textStatus, errorThrown) {
						ShowInfo(jqXHR.responseText);
					}
				});
			}

		}else{
			alert("Data: " + data + "\nStatus: " + status);
		}
	});
}

function getDevName(){
	$.get("/system",function(data, status){
		if (status == "success"){
			$("#devName").text("智能插座 （" + data.PlugName + "）");
			$("#title").text(data.PlugName);
		 }else{
			alert("Data: " + data + "\nStatus: " + status);
		}
	});
}

function getInfomation(){
	$.get("/info",function(data, status){
		if (status == "success"){
			BuildDate = "编译时间 ："+ data.BuildDate;
			SDKVersion = "固件版本 ：" + data.SDKVersion;
			FlashMap = "Flash大小 ：" + data.FlashMap;
			UserBin = "当前固件 ： " + data.UserBin;
			var day = parseInt(data.RunTime/(24*3600));
			var hour= parseInt((data.RunTime%(24*3600))/3600);
			var min = parseInt((data.RunTime%3600)/60);
			var sec = parseInt(data.RunTime%60);
			RunTime = "运行时间 ：" + day+ " 天 "+ hour +" 时 "+ min +" 分 "+ sec +" 秒 ";
			$("#aboutBody").empty();
			$("#aboutBody").append("<li>"+RunTime+"</li>");
			$("#aboutBody").append("<li>"+BuildDate+"</li>");
			$("#aboutBody").append("<li>"+SDKVersion+"</li>");
			$("#aboutBody").append("<li>"+UserBin+"</li>");
			$("#aboutBody").append("<li>"+FlashMap+"</li>");

			$("#aboutBody").append("</br>&copy;&nbsp;2019&nbsp;sunkai.mr@qq.com");

			$('#aboutModal').modal("show")

		}else{
			alert("Data: " + data + "\nStatus: " + status);
		}
	});
}

function setRelayStyle(status) {
	if ( status ) {
		relaystatus = true;
		$("#relay").text("开启");
	}else{
		relaystatus = false;
		$("#relay").text("关闭");
	}
}

function refreshRelay(){
	$.get("/relaystatus",function(data, status){
		if (status == "success"){
			if (data.status == "on"){
				setRelayStyle( true );
			}else{
				setRelayStyle( false );
			}
		}else{
			alert("Data: " + data + "\nStatus: " + status);
		}
	});
}

function setRelay(){
    $.get("/relaystatus",function(data, status){
        if (status == "success"){
            if (data.status == "on"){
                str = "{\"status\":\"off\"}";
            }else{
                str = "{\"status\":\"on\"}";
            }

            $.ajax({
                type: "POST",
                url: "/relaystatus",
                contentType: "application/json",
                dataType: "json",
                data: str,
                success: function (data) {
                    if (data.status == "on"){
                        setRelayStyle( true );
                    }else{
                        setRelayStyle( false );
                    }
                }
            });

        }else{
            ShowInfo("Data: " + data + "\nStatus: " + status);
        }
    });
}

function BoolConversion( b ) {
  if ( b ){
	  return "是";
  }else{
	  return "否";
  }
}

function weekConversionString( week ) {
	if ( week == 0 ){
		return "执行一次";
	}

	if ( week == 127 ){
		return "每天";
	}

	if ( week == 31 ){
		return "工作日";
	}

	if ( week == 96 ){
		return "周末";
	}

	var str="";
	for ( var i = 0; i < 7; i++ ){
		switch(i){
			case 0: if(week&(1<<i)){str=str+"一、";}break;
			case 1: if(week&(1<<i)){str=str+"二、";}break;
			case 2: if(week&(1<<i)){str=str+"三、";}break;
			case 3: if(week&(1<<i)){str=str+"四、";}break;
			case 4: if(week&(1<<i)){str=str+"五、";}break;
			case 5: if(week&(1<<i)){str=str+"六、";}break;
			case 6: if(week&(1<<i)){str=str+"日、";}break;
		}
	}
	return str.substring(0,str.length-1);
}

function stringConversionWeek() {
	var week = 0;

	if( document.getElementById("week1").checked==true ){week=week|(1<<0)}
	if( document.getElementById("week2").checked==true ){week=week|(1<<1)}
	if( document.getElementById("week3").checked==true ){week=week|(1<<2)}
	if( document.getElementById("week4").checked==true ){week=week|(1<<3)}
	if( document.getElementById("week5").checked==true ){week=week|(1<<4)}
	if( document.getElementById("week6").checked==true ){week=week|(1<<5)}
	if( document.getElementById("week7").checked==true ){week=week|(1<<6)}

	return week;
}

function TimerClick(){

	$("#timer").addClass("lead");
	$("#delay").removeClass("lead");
	$("#set").removeClass("lead");
	$("#tabDelay").addClass("hidden");
	$("#formSet").addClass("hidden");
    $("#delayTaskClass").addClass("hidden");
	$("#tabTimer").removeClass("hidden");
	$("#tabTimer").html("<p>正在加载数据...</p>");

	$.get("timer/all",function(data, status){
		if (status == "success"){

			$("#tabTimer").empty();
			$("#tabTimer").html("<thead><th>编号</th><th>名称</th><th>启用</th><th>开启时间</th><th>关闭时间</th><th>关联延时</th><th>重复</th><th></th></thead><tbody><tr></tr></tbody>");

			data = eval(data);
			var t = document.getElementById('tabTimer');
			$.each(data, function (index, item) {
				var r = t.insertRow(t.rows.length);
				for( var i = 0; i < 8; i++){
					switch (i){
						case 0: r.insertCell(i).innerHTML=item.Num;break;
						case 1: r.insertCell(i).innerHTML=item.Name;break;
						case 2: r.insertCell(i).innerHTML=BoolConversion(item.Enable);break;
						case 3: if (item.OnEnable){
                                    r.insertCell(i).innerHTML=item.OnTime+" / Y";
                                }else{
                                    r.insertCell(i).innerHTML=item.OnTime+" / N";
                                }
                                break;
						case 4: if (item.OffEnable){
                                    r.insertCell(i).innerHTML=item.OffTime+" / Y";
                                }else{
                                    r.insertCell(i).innerHTML=item.OffTime+" / N";
                                }
                                break;
						case 5: if (item.Cascode){
                                    r.insertCell(i).innerHTML=item.CascodeNum+" / Y";
                                }else{
                                    r.insertCell(i).innerHTML=item.CascodeNum+" / N";
                                }
                                break;
						case 6: r.insertCell(i).innerHTML=weekConversionString(item.Week);break;
						case 7: r.insertCell(i).innerHTML="<a>修改</a>";break;
						default:break;
					}
				}
			});
			$("#tabTimer a").click(tabTimerSubmit);
		}else{
			alert("Data: " + data + "\nStatus: " + status);
		}
	});
}

function DelayClick(){
	$("#timer").removeClass("lead");
	$("#delay").addClass("lead");
	$("#set").removeClass("lead");
	$("#formSet").addClass("hidden");
	$("#tabDelay").removeClass("hidden");
	$("#tabTimer").addClass("hidden");
	$("#tabDelay").html("<p><centor>正在加载数据...</centor></p>");

	$.get("delay/all",function(data, status){
		if (status == "success"){
			$("#tabDelay").empty();
			$("#tabDelay").html("<thead><th>编号</th><th>名称</th><th>启用</th><th>开启间隔</th><th>关闭间隔</th><th>关联延时</th><th>重复次数</th><th></th></thead><tbody><tr></tr></tbody>");
            DelayData = data;
			data = eval(data);
			var t = document.getElementById('tabDelay');
			$.each(data, function (index, item) {
				var r = t.insertRow(t.rows.length);
				for( var i = 0; i < 8; i++){
					switch (i){
						case 0: r.insertCell(i).innerHTML=item.Num;break;
						case 1: r.insertCell(i).innerHTML=item.Name;break;
						case 2: r.insertCell(i).innerHTML=BoolConversion(item.Enable);break;
						case 3: if (item.OnEnable){
                                    r.insertCell(i).innerHTML=item.OnInterval+" / Y";
                                }else{
                                    r.insertCell(i).innerHTML=item.OnInterval+" / N";
                                }
                                break;
						case 4: if (item.OffEnable){
                                    r.insertCell(i).innerHTML=item.OffInterval+" / Y";
                                }else{
                                    r.insertCell(i).innerHTML=item.OffInterval+" / N";
                                }
                                break;
						case 5: if (item.Cascode){
                                    r.insertCell(i).innerHTML=item.CascodeNum+" / Y";
                                }else{
                                    r.insertCell(i).innerHTML=item.CascodeNum+" / N";
                                }
                                break;
						case 6: if (item.Enable){
									r.insertCell(i).innerHTML=item.TmpCycleTimes;
								}else{
									r.insertCell(i).innerHTML=item.CycleTimes;
								}
								break;
						case 7: r.insertCell(i).innerHTML="<a>修改</a>";break;
						default:break;
					}
				}
			});
			$("#tabDelay a").click(tabDelaySubmit);
            refreshTask();
		}else{
			alert("Data: " + data + "\nStatus: " + status);
		}
	});
}

function SetClick(){
	$("#tabTimer").addClass("hidden");
	$("#tabDelay").addClass("hidden");
    $("#delayTaskClass").addClass("hidden");
	$("#formSet").removeClass("hidden");
	$("#timer").removeClass("lead");
	$("#delay").removeClass("lead");
	$("#set").addClass("lead");

	$.get("/system",function(data, status){
		if (status == "success"){
            SysData = data;

			var options=document.getElementById("modeSelect").options;
			options[data.WifiMode].selected = true;

			$("#wifiList").empty();
			var opt = $("<option>").val(1).text(data.WifiSSID);
			opt.selected = true;
			$("#wifiList").append(opt);

            var options=document.getElementById("PlatformSelect").options;
            options[data.CloudPlatform].selected = true;

            document.getElementById("plugName").value=data.PlugName;
            document.getElementById("wifiCustom").value=data.WifiSSID;
			document.getElementById("wifiPasswd").value=data.WifiPasswd;
			document.getElementById("ProductKey").value=data.MqttProductKey;
			document.getElementById("DeviceName").value=data.MqttDevName;
			document.getElementById("DeviceSecret").value=data.MqttDevSecret;
            document.getElementById("BigiotDeviceId").value=data.BigiotDevId;
            document.getElementById("BigiotApiKey").value=data.BigiotApiKey;
			modeChange();

		}else{
			alert("Data: " + data + "\nStatus: " + status);
		}
	});
}

function chooseBinClick(){
	$('#file').click();
}

function rebootClick(){
	$('#rebootModal').modal("show")
}

function reboot(){
	 $.ajax({
		 type: "POST",
		 url: "/control",
		 contentType: "application/json",
		 dataType: "json",
		 data: "{\"Action\":0}",
		 success: function (data) {
			 ShowInfo("设备已重启成功")
		 }
	 });
	$('#rebootModal').modal("toggle")
}

function resetClick(){
	$('#resetModal').modal("show")
}

function reset(){
	$.ajax({
		type: "POST",
		url: "/control",
		contentType: "application/json",
		dataType: "json",
		data: "{\"Action\":1}",
		success: function (data) {
			ShowInfo("设备已恢复到出厂模式")
		}
	});
	$('#resetModal').modal("toggle")
}

function uploadClick() {

	var file = document.getElementById("file").files[0];
	if ($('#file').val() == "") {
		ShowInfo("请选择固件");
		return;
	}

	fileName = file.name;
	fileExt = fileName.substr(fileName.lastIndexOf(".")).toLowerCase();
	if (fileExt != '.bin') {
		ShowInfo("文件类型错误，请选择bin文件。");
		return;
	}

	xhr = new XMLHttpRequest();
	xhr.open("put", "/upgrade", true);
	xhr.upload.onprogress = progressFunction;
	xhr.send(file);
	xhr.onreadystatechange = function () {
		if (xhr.readyState == 4) {
			if (xhr.status == 201) {
				ShowInfo("升级成功，设备正在重新启动");
			} else {
				ShowInfo("升级失败");
			}
		}
	}
}

function progressFunction(evt) {
	if (evt.lengthComputable) {
		ShowInfo( "正在升级 " + Math.round(evt.loaded / evt.total * 100) + "%");
	}
}

function tabTimerSubmit(){
	numStr = $(this).parents("tr").find('td').eq(0).text();
	url="timer/"+numStr;
	$.get(url, function(data, status){
		if (status == "success"){
			$('#timerModalHead').text("定时 " + data[0].Num);
			document.getElementById("timeName").value=data[0].Name;
			document.getElementById("timeEnable").checked=data[0].Enable;
			document.getElementById("onTime").value=data[0].OnTime;
			document.getElementById("onTimeEnable").checked = data[0].OnEnable;
			document.getElementById("offTimeEnable").checked = data[0].OffEnable;
			document.getElementById("offTime").value=data[0].OffTime;
			document.getElementById("timerCascodeNum").value=data[0].CascodeNum;
			document.getElementById("timerCascodeEnable").checked =data[0].Cascode;
			var week= parseInt(data[0].Week);
			for ( var i = 0; i < 7; i++ ){
				switch(i){
					case 0: if (week&(1<<i)){document.getElementById("week1").checked=true;}else{document.getElementById("week1").checked=false;}break;
					case 1: if (week&(1<<i)){document.getElementById("week2").checked=true;}else{document.getElementById("week2").checked=false;}break;
					case 2: if (week&(1<<i)){document.getElementById("week3").checked=true;}else{document.getElementById("week3").checked=false;}break;
					case 3: if (week&(1<<i)){document.getElementById("week4").checked=true;}else{document.getElementById("week4").checked=false;}break;
					case 4: if (week&(1<<i)){document.getElementById("week5").checked=true;}else{document.getElementById("week5").checked=false;}break;
					case 5: if (week&(1<<i)){document.getElementById("week6").checked=true;}else{document.getElementById("week6").checked=false;}break;
					case 6: if (week&(1<<i)){document.getElementById("week7").checked=true;}else{document.getElementById("week7").checked=false;}break;
				}
			}
		}else{
			ShowInfo("Data: " + data + "\nStatus: " + status);
		}});

	$('#timerSubmitModal').modal("show")
}

function tabDelaySubmit(){
	numStr = $(this).parents("tr").find('td').eq(0).text();
	url="delay/"+numStr;
	$.get(url, function(data, status){
		if (status == "success"){
			$('#delayModalHead').text("延时 " + data[0].Num);
			document.getElementById("delayName").value=data[0].Name;
			document.getElementById("delayEnable").checked=data[0].Enable;
			document.getElementById("onInterval").value=data[0].OnInterval;
			document.getElementById("onIntervalEnable").checked = data[0].OnEnable;
			document.getElementById("offIntervalEnable").checked = data[0].OffEnable;
			document.getElementById("offInterval").value=data[0].OffInterval;
			document.getElementById("delayCascodeNum").value=data[0].CascodeNum;
			document.getElementById("delayCascodeEnable").checked =data[0].Cascode;
			document.getElementById("cycleTimes").value =data[0].CycleTimes;
		}else{
			alert("Data: " + data + "\nStatus: " + status);
		}});

	$('#delaySubmitModal').modal("show")
}

function timerSubmit() {
	$('#timerSubmitForm').bootstrapValidator();
}

function ShowInfo( info ){
	$("#infoBody").text(info);
	$('#infoModal').modal("show")
}

function setCommitClick(){
	var data = {};
	data.PlugName=document.getElementById("plugName").value;
	if (data.PlugName.length > 32 ){
		ShowInfo("名称超过长度限制");
		return;
	}

	$("#devName").text("智能插座 （" + data.PlugName + "）");
	$("#title").text(data.PlugName);
	if ( document.getElementById("modeSelect").value == 2 ){
		data.WifiMode= 2;
	}else if ( document.getElementById("modeSelect").value == 1 ){
		data.WifiMode= 1;

        if (SysData.WifiMode == 2) {
            data.WifiSSID=document.getElementById("wifiCustom").value;
        }else{
            data.WifiSSID=$("#wifiList option:selected").text();
        }

		data.WifiPasswd=document.getElementById("wifiPasswd").value;
		data.SmartConfigFlag=true;
	}else if ( document.getElementById("modeSelect").value == 3 ){
		data.WifiMode = 1;
		data.SmartConfigFlag = false;
	}

	if ( data.WifiMode == 1 ){
        data.CloudPlatform = parseInt(document.getElementById("PlatformSelect").value);
        if ( data.CloudPlatform == 1 ){
            data.MqttProductKey = document.getElementById("ProductKey").value;
            data.MqttDevName = document.getElementById("DeviceName").value;
            data.MqttDevSecret = document.getElementById("DeviceSecret").value;
        }else if ( data.CloudPlatform == 2 ){
            data.BigiotDevId = document.getElementById("BigiotDeviceId").value;
            data.BigiotApiKey = document.getElementById("BigiotApiKey").value;
        }

	}
	$.ajax({
		type: "POST",
		url: "/system",
		contentType: "application/json",
		dataType: "json",
		data: JSON.stringify(data),
		success: function (data) {
			ShowInfo("修改成功，重启生效");
		},
		error: function (jqXHR, textStatus, errorThrown) {
			ShowInfo(jqXHR.responseText);
		}
	});
}

function scanWifiClick(){
	$("#wifiList").empty();
	document.getElementById("wifiPasswd").value = "";
	var opt = $("<option>").val(1).text("正在扫描...");
	opt.selected = true;
	$("#wifiList").append(opt);

	$.get("/scanwifi",function(data, status){
		if (status == "success"){
			$("#wifiList").empty();
			$.each(data, function (index, item) {
				var opt = $("<option>").val(1).text(item.Ssid);
				$("#wifiList").append(opt);
			});
		}else{
			ShowInfo("Data: " + data + "\nStatus: " + status);
		}
	});
}

function refreshTask() {
    var index = 0;
    data = eval(DelayData);
    $.each(data, function (i, item) {
        if (item.Enable && (item.OnEnable||item.OffEnable)){
            $("#delayTaskClass").removeClass("hidden");
            DelayRefreshTime = item.TimePoint;
            var action;
            switch(item.SwFlag){
                case 1: action = " 开启，还剩 " + item.TmpCycleTimes + " 次";break;
                case 2: action = " 关闭，还剩 " + item.TmpCycleTimes + " 次";break;
                case 0: if(item.Cascode){
                            action = " 结束，并开始执行 "+data[item.CascodeNum-1].Name;
                        }else{
                            action = " 结束";
                        }
                        break;
            }
            $("#delayTask").text("提示："+item.Name+" 将在 "+item.TimePoint+ action);
            return false;
        }
        index++;
    });
    if(index >= data.length){
        $("#delayTaskClass").addClass("hidden");
        $("#delayTask").text("提示：无");
        DelayRefreshTime = "";
    }
}
