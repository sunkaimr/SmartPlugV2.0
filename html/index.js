
var LocalTime = 0;
var relaystatus = false;
var SysData;
var MeterData;
var DelayData;
var InfraredData;
var DelayRefreshTime="";
var meterRefreshTimer;
var webSet;
var ModelTab="";
var dateStr="";

$(document).ready(function () {

    getWebSet();
    getDate();
	getDevName();
    GetTemperaturer();
	refreshRelay();
	setInterval(DateDisplay, 1000);

	$("#relay").click(setRelay);
	$("#date").click(getDate);
    $("#console").click(openConsole);
	$("#timer").click(TimerClick);
    $("#temperature").click(GetTemperaturer);
	$("#delay").click(DelayClick);
    $("#infrared").click(InfraredClick);
    $("#meter").click(MeterClick);
    $("#cloudPlatform").click(CloudPlatformClick);
	$("#set").click(SetClick);
	$("#setCommit").click(setCommitClick);
	$("#cloudPlatformCommit").click(cloudPlatformCommitClick);
	$("#binFile").click(chooseBinClick);
	$("#scanWifi").click(scanWifiClick);
	$("#upload").click(uploadClick);
	$("#reboot").click(rebootClick);
    $("#meterClear").click(meterClearClick);
	$("#reset").click(resetClick);
	$("#modeSelect").change(modeChange);
    $("#PlatformSelect").change(PlatformSelectChange);
	$("#file").change(binFileChange);
	$("#information").click(getInfomation);
	$("#rebootModalButton").click(reboot);
	$("#resetModalButton").click(reset);
    $("#meterClearModalButton").click(meterClear);
    $("#meterRefreshSelect").change(meterRefreshChange);
    $("#meterButton").click(meterSubmit);

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
			jsonData.Name = $("#timeName").val();
			jsonData.Enable = $("#timeEnable").is(':checked');
			jsonData.OnEnable = $("#onTimeEnable").is(':checked');
			jsonData.OffEnable = $("#offTimeEnable").is(':checked');
			jsonData.Cascode = $("#timerCascodeEnable").is(':checked');
			jsonData.CascodeNum = parseInt($("#timerCascodeNum").val());
			jsonData.Week = stringConversionWeek();
			jsonData.OnTime = $("#onTime").val();
			jsonData.OffTime = $("#offTime").val();
			jsonDataAry.push(jsonData);

			$.ajax({
				url: $form.attr('action'),
				type: "POST",
				contentType: "application/json",
				data:JSON.stringify(jsonDataAry),
				success: function () {
                    TimerClick();
					//alert("成功");
				}
			});
			$("#timerSubmitModal").modal("toggle");
			$("#timerSubmitBtn").attr('disabled', false);

		});
	});

	$(function () {
		$('#infraredSubmitForm').bootstrapValidator({
			message: 'This value is not valid',
			feedbackIcons: {
				valid: 'glyphicon glyphicon-ok',
				invalid: 'glyphicon glyphicon-remove',
				//validating: 'glyphicon glyphicon-refresh'
			},
			fields: {
                infraredName:{
					message: 'infrared name is not valid',
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
                infraredOnValue: {
					message: 'infraredOnValue is invalid',
					validators: {
						notEmpty: {
							message: '开启值不能为空'
						},
					}
				},
                infraredOffValue: {
					message: 'infraredOffValue is invalid',
					validators: {
						notEmpty: {
							message: '关闭值不能为空'
						},
					}
				}
			}
		}).on('success.form.bv', function (e) {//点击提交之后
			e.preventDefault();
			var $form = $(e.target);
			var jsonDataAry=[];
			var jsonData = {};

			var numStr=$('#infraredModalHead').text().trim().split(/\s+/);
			jsonData.Num = parseInt(numStr[numStr.length-1]);
			jsonData.Name = $("#infraredName").val();
			jsonData.Enable = $("#infraredEnable").is(':checked');
            jsonData.OnValue = $("#infraredOnValue").val();
            jsonData.OffValue = $("#infraredOffValue").val();
			jsonDataAry.push(jsonData);

			$.ajax({
				url: $form.attr('action'),
				type: "POST",
				contentType: "application/json",
				data:JSON.stringify(jsonDataAry),
				success: function () {
                    InfraredClick();
				},
				error: function (jqXHR, textStatus, errorThrown) {
					ShowInfo(jqXHR.responseText);
				}
			});

			$("#infraredSubmitModal").modal("toggle");
            $("#infraredSubmitBtn").attr('disabled', false);
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
            jsonData.Name = $("#delayName").val();
            jsonData.Enable = $("#delayEnable").is(':checked');
            jsonData.OnEnable = $("#onIntervalEnable").is(':checked');
            jsonData.OffEnable = $("#onIntervalEnable").is(':checked');
            jsonData.Cascode = $("#delayCascodeEnable").is(':checked');
            jsonData.CascodeNum = parseInt($("#delayCascodeNum").val());
            jsonData.CycleTimes = parseInt($("#cycleTimes").val());
            jsonData.OnInterval = $("#onInterval").val();
            jsonData.OffInterval = $("#offInterval").val();
            jsonDataAry.push(jsonData);

            $.ajax({
                url: $form.attr('action'),
                type: "POST",
                contentType: "application/json",
                data:JSON.stringify(jsonDataAry),
                success: function () {
                    DelayClick();
                    //alert("成功");
                },
                error: function (jqXHR, textStatus, errorThrown) {
                    ShowInfo(jqXHR.responseText);
                }
            });

            $("#delaySubmitModal").modal("toggle");
            $("#delaySubmitBtn").attr('disabled', false);
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
            $("#wifiCustomClass, #wifiPasswdClass").removeClass("hidden");
        }else{
            $("#wifiClass, #wifiPasswdClass").removeClass("hidden");
        }
	}else{
		$("#wifiClass, #wifiPasswdClass, #wifiCustomClass").addClass("hidden");
	}
}

function PlatformSelectChange(){

	$("#PlatformSelectClass").removeClass("hidden");

	if( $("#PlatformSelect").val() == 1 ) {

		$("#ProductKeyClass, #DeviceNameClass, #DeviceSecretClass").removeClass("hidden");

		$("#BigiotDeviceIdClass, #BigiotApiKeyClass, #BigiotDeviceTypeClass, #BigiotDeviceNameClass").addClass("hidden");
		$("#BigiotDeviceTypeClass, #BigiotIfSwitchStatusClass, #BigiotIfTempClass, #BigiotIfHumidityClass").addClass("hidden");
        $("#BigiotIfVoltageClass, #BigiotIfCurrentClass, #BigiotIfPowerClass, #BigiotIfElectricityClass").addClass("hidden");

	}else if (  $("#PlatformSelect").val() == 2 ){

		$("#ProductKeyClass, #DeviceNameClass, #DeviceSecretClass").addClass("hidden");
        $("#BigiotDeviceNameClass, #BigiotApiKeyClass, #BigiotDeviceTypeClass").removeClass("hidden");
        $("#BigiotDeviceIdClass, #BigiotIfSwitchStatusClass, #BigiotIfTempClass, #BigiotIfHumidityClass").removeClass("hidden");
        $("#BigiotIfVoltageClass, #BigiotIfCurrentClass, #BigiotIfPowerClass, #BigiotIfElectricityClass").removeClass("hidden");
	}else{
        $("#ProductKeyClass, #DeviceNameClass, #DeviceSecretClass").addClass("hidden");
        $("#BigiotDeviceNameClass, #BigiotApiKeyClass, #BigiotDeviceTypeClass").addClass("hidden");
        $("#BigiotDeviceIdClass, #BigiotIfSwitchStatusClass, #BigiotIfTempClass, #BigiotIfHumidityClass").addClass("hidden");
        $("#BigiotIfVoltageClass, #BigiotIfCurrentClass, #BigiotIfPowerClass, #BigiotIfElectricityClass").addClass("hidden");
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
        GetTemperaturer();
        getDate();
	}else if(parseInt(LocalTime%60) == 3){
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
            dateStr=(data.Date).replace(/\-/g, "/");
			if ( data.SyncTime ){
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

function openConsole(){
    var url = "http://"+`${document.location.host}`+"/console.html";
    window.open(url, "_blank");
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

function getWebSet(){
    $.get("/webset",function(data, status){
        if (status == "success"){
            webSet = data
			switch (webSet.ModelTab) {
				case "timer":
                    TimerClick();
					break;
                case "delay":
                    DelayClick();
                    break;
                case "infrared":
                    InfraredClick();
                    break;
                case "meter":
                    MeterClick();
                    break;
                case "cloudplatform":
                    CloudPlatformClick();
                    break;
                case "sysset":
                    SetClick();
                    break;
				default:
                    TimerClick();
					break;
			}
        }else{
            TimerClick();
            alert("Data: " + data + "\nStatus: " + status);
        }
    });
}

function setWebSet(){
    var data = {};
    data.ModelTab = ModelTab;
    data.MeterRefresh = $("#meterRefreshSelect").val().toString();
    if ( data.MeterRefresh == webSet.MeterRefresh && data.ModelTab == webSet.ModelTab ){
        return;
    }

    $.ajax({
        type: "POST",
        url: "/webset",
        contentType: "application/json",
        dataType: "json",
        data: JSON.stringify(data),
        success: function (data) {
            webSet = data
        },
        error: function (jqXHR, textStatus, errorThrown) {

        }
    });
}

function getInfomation(){
	$.get("/info",function(data, status){
		if (status == "success"){
            GitCommit = "GitCommit ："+ data.GitCommit;
			BuildDate = "编译时间 ："+ data.BuildDate;
			SDKVersion = "SDK版本 ：" + data.SDKVersion;
			FlashMap = "Flash大小 ：" + data.FlashMap;
			UserBin = "当前固件 ： " + data.UserBin;
			var day = parseInt(data.RunTime/(24*3600));
			var hour= parseInt((data.RunTime%(24*3600))/3600);
			var min = parseInt((data.RunTime%3600)/60);
			var sec = parseInt(data.RunTime%60);
			RunTime = "运行时间 ：" + day+ " 天 "+ hour +" 时 "+ min +" 分 "+ sec +" 秒 ";
			$("#aboutBody").empty();
			$("#aboutBody").append("<li>"+RunTime+"</li>");
            $("#aboutBody").append("<li>"+GitCommit+"</li>");
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


function GetTemperaturer(){
    $.get("/temperature",function(data, status){
        if (status == "success"){
            $("#temperature").text(data.Temperature.toFixed(1) + " ℃");
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
		return "一次";
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

	if( $("#week1").is(':checked') ){week=week|(1<<0)}
	if( $("#week2").is(':checked') ){week=week|(1<<1)}
	if( $("#week3").is(':checked') ){week=week|(1<<2)}
	if( $("#week4").is(':checked') ){week=week|(1<<3)}
	if( $("#week5").is(':checked') ){week=week|(1<<4)}
	if( $("#week6").is(':checked') ){week=week|(1<<5)}
	if( $("#week7").is(':checked') ){week=week|(1<<6)}
	return week;
}

function TimerClick(){
    clearInterval(meterRefreshTimer);

	$("#delay, #infrared, #cloudPlatform, #set, #meter").removeClass("lead");
    $("#timer").addClass("lead");

	$("#tabDelay, #tabInfrared, #formSet, #formCloudPlatform, #delayTaskClass, #meterInfo").addClass("hidden");

	$("#tabTimer").removeClass("hidden");
	$("#tabTimer").html("<p>正在加载数据...</p>");

    ModelTab = "timer"
    setWebSet();
	$.get("timer/all",function(data, status){
		if (status == "success"){

			$("#tabTimer").empty();
			$("#tabTimer").html("<thead><th>编号</th><th>名称</th><th>开启时间</th><th>关闭时间</th><th>关联延时</th><th>重复</th><th></th></thead><tbody><tr></tr></tbody>");

			data = eval(data);
			var t = document.getElementById('tabTimer');
			$.each(data, function (index, item) {
				var r = t.insertRow(t.rows.length);
                if (item.Enable){
                    r.style.background = "#33FFCC";
                }
				for( var i = 0; i < 7; i++){
					switch (i){
						case 0: r.insertCell(i).innerHTML=item.Num;break;
						case 1: r.insertCell(i).innerHTML=item.Name;break;
						case 2: if (item.OnEnable){
                                    r.insertCell(i).innerHTML=item.OnTime+" / Y";
                                }else{
                                    r.insertCell(i).innerHTML=item.OnTime+" / N";
                                }
                                break;
						case 3: if (item.OffEnable){
                                    r.insertCell(i).innerHTML=item.OffTime+" / Y";
                                }else{
                                    r.insertCell(i).innerHTML=item.OffTime+" / N";
                                }
                                break;
						case 4: if (item.Cascode){
                                    r.insertCell(i).innerHTML=item.CascodeNum+" / Y";
                                }else{
                                    r.insertCell(i).innerHTML=item.CascodeNum+" / N";
                                }
                                break;
						case 5: r.insertCell(i).innerHTML=weekConversionString(item.Week);break;
						case 6: r.insertCell(i).innerHTML="<a>修改</a>";break;
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
    clearInterval(meterRefreshTimer);

    $("#timer, #infrared, #cloudPlatform, #set, #meter").removeClass("lead");
	$("#delay").addClass("lead");

	$("#formSet, #tabTimer, #formCloudPlatform, #tabInfrared, #meterInfo").addClass("hidden");
	$("#tabDelay").removeClass("hidden");
	$("#tabDelay").html("<p><centor>正在加载数据...</centor></p>");
    ModelTab = "delay"
    setWebSet();
	$.get("delay/all",function(data, status){
		if (status == "success"){
			$("#tabDelay").empty();
			$("#tabDelay").html("<thead><th>编号</th><th>名称</th><th>开启间隔</th><th>关闭间隔</th><th>关联延时</th><th>重复次数</th><th></th></thead><tbody><tr></tr></tbody>");
            DelayData = data;
			data = eval(data);
			var t = document.getElementById('tabDelay');
			$.each(data, function (index, item) {
				var r = t.insertRow(t.rows.length);
                if (item.Enable){
                    r.style.background = "#33FFCC";
                }
				for( var i = 0; i < 7; i++){
					switch (i){
						case 0: r.insertCell(i).innerHTML=item.Num;break;
						case 1: r.insertCell(i).innerHTML=item.Name;break;
						case 2: if (item.OnEnable){
                                    r.insertCell(i).innerHTML=item.OnInterval+" / Y";
                                }else{
                                    r.insertCell(i).innerHTML=item.OnInterval+" / N";
                                }
                                break;
						case 3: if (item.OffEnable){
                                    r.insertCell(i).innerHTML=item.OffInterval+" / Y";
                                }else{
                                    r.insertCell(i).innerHTML=item.OffInterval+" / N";
                                }
                                break;
						case 4: if (item.Cascode){
                                    r.insertCell(i).innerHTML=item.CascodeNum+" / Y";
                                }else{
                                    r.insertCell(i).innerHTML=item.CascodeNum+" / N";
                                }
                                break;
						case 5: if (item.Enable){
									r.insertCell(i).innerHTML=item.TmpCycleTimes;
								}else{
									r.insertCell(i).innerHTML=item.CycleTimes;
								}
								break;
						case 6: r.insertCell(i).innerHTML="<a>修改</a>";break;
						default:break;
					}
				}
			});
			$("#tabDelay a").click(tabDelaySubmit);
            getDate();
            refreshTask();
		}else{
			alert("Data: " + data + "\nStatus: " + status);
		}
	});
}


function InfraredClick(){
    clearInterval(meterRefreshTimer);

    $("#timer, #delay, #cloudPlatform, #set, #meter").removeClass("lead");
    $("#infrared").addClass("lead");

    $("#formSet, #tabDelay, #formCloudPlatform, #tabTimer, #meterInfo").addClass("hidden");
    $("#tabInfrared").removeClass("hidden");

    $("#tabInfrared").html("<p><centor>正在加载数据...</centor></p>");
    ModelTab = "infrared"
    setWebSet();
    $.get("infrared/all",function(data, status){
        if (status == "success"){
            $("#tabInfrared").empty();
            $("#tabInfrared").html("<thead><th>编号</th><th>名称</th><th>开启值</th><th>关闭值</th></thead><tbody><tr></tr></tbody>");
            InfraredData = data;
            data = eval(data);
            var t = document.getElementById('tabInfrared');
            $.each(data, function (index, item) {
                var r = t.insertRow(t.rows.length);
                if (item.Enable){
                    r.style.background = "#33FFCC";
                }
                for( var i = 0; i < 5; i++){
                    switch (i){
                        case 0: r.insertCell(i).innerHTML=item.Num;break;
                        case 1: r.insertCell(i).innerHTML=item.Name;break;
                        case 2: r.insertCell(i).innerHTML=item.OnValue;break;
                        case 3: r.insertCell(i).innerHTML=item.OffValue;break;
                        case 4: r.insertCell(i).innerHTML="<a>修改</a>";break;
                        default: break;
                    }
                }
            });
            $("#tabInfrared a").click(tabInfraredSubmit);

        }else{
            alert("Data: " + data + "\nStatus: " + status);
        }
    });
}


function CloudPlatformClick(){
    clearInterval(meterRefreshTimer);

    $("#timer, #delay, #infrared, #set, #meter").removeClass("lead");
    $("#cloudPlatform").addClass("lead");

    $("#formSet, #tabDelay, #tabTimer, #tabInfrared, #meterInfo").addClass("hidden");
	$("#formCloudPlatform").removeClass("hidden");
    ModelTab = "cloudplatform"
    setWebSet();
    $.get("/cloudplatform",function(data, status){
        if (status == "success"){
            $("#PlatformSelect").val(data.CloudPlatform);
            $("#ProductKey").val(data.MqttProductKey);
            $("#DeviceName").val(data.MqttDevName);
            $("#DeviceSecret").val(data.MqttDevSecret);
            $("#BigiotDeviceName").val(data.BigiotDevName);
            $("#BigiotDeviceId").val(data.BigiotDevId);
            $("#BigiotApiKey").val(data.BigiotApiKey);
            $("#BigiotIfSwitchStatus").val(data.SwitchId)
            $("#BigiotIfTemp").val(data.TempId)
            $("#BigiotIfHumidity").val(data.HumidityId)
            $("#BigiotDeviceTypeSelect").val(data.DevType);
            $("#BigiotIfVoltage").val(data.VoltageId);
            $("#BigiotIfCurrent").val(data.CurrentId);
            $("#BigiotIfPower").val(data.PowerId);
            $("#BigiotIfElectricity").val(data.ElectricityId);

            var str = "";
            if ( data.ConnectSta == "connected" ){
                str = "已连接至" + $("#PlatformSelect option:selected").text()+"平台";
			}else if ( data.ConnectSta == "disconnect" ){
                str = "连接" + $("#PlatformSelect option:selected").text() + "失败";
			}else{
                str = "连接状态未知";
			}
            $("#connectSta").text(str);

            PlatformSelectChange();
        }else{
            alert("Data: " + data + "\nStatus: " + status);
        }
    });

}


function SetClick(){
    clearInterval(meterRefreshTimer);

    $("#timer, #delay, #infrared, #cloudPlatform, #meter").removeClass("lead");
    $("#set").addClass("lead");

	$("#tabTimer, #tabDelay, #tabInfrared, #delayTaskClass, #formCloudPlatform, #meterInfo").addClass("hidden");
	$("#formSet").removeClass("hidden");
    ModelTab = "sysset"
    setWebSet();

	$.get("/system",function(data, status){
		if (status == "success"){
            SysData = data;

            $("#modeSelect").val(data.WifiMode);
            $("#relayPowerUp").val(data.RelayPowerUp);
            $("#wifiList").empty();
			var opt = $("<option>").val(1).text(data.WifiSSID);
			opt.selected = true;
			$("#wifiList").append(opt);
            $("#plugName").val(data.PlugName)
            $("#wifiCustom").val(data.WifiSSID);
			$("#wifiPasswd").val(data.WifiPasswd);

			modeChange();

		}else{
			alert("Data: " + data + "\nStatus: " + status);
		}
	});
}

function MeterClick(){
    $("#timer, #delay, #infrared, #cloudPlatform, #set").removeClass("lead");
    $("#meter").addClass("lead");

    $("#tabTimer, #tabDelay, #tabInfrared, #delayTaskClass, #formCloudPlatform, #formSet").addClass("hidden");
    $("#meterInfo").removeClass("hidden");

    $("#meterRefreshSelect").val(parseInt(webSet.MeterRefresh))
    ModelTab = "meter"
    meterRefreshChange();
    getMeterAllInfo();
}

function getMeterInfo(){
    $.get("/meter",function(data, status){
        if (status == "success"){
            MeterData = data;
            $("#meterVoltage").val(MeterData.Voltage)
            $("#meterCurrent").val(MeterData.Current)
            $("#meterPower").val(MeterData.Power)
            $("#meterApparentPower").val(MeterData.ApparentPower)
            $("#meterPowerFactor").val(MeterData.PowerFactor)
            $("#meterElectricity").val(MeterData.Electricity)
            $("#meterRunTime").val(MeterData.RunTime)
        }else{
            alert("Data: " + data + "\nStatus: " + status);
        }
    });
}

function getMeterAllInfo(){
    $.get("/meter",function(data, status){
        if (status == "success"){
            MeterData = data;
            refreshMeterInfo();
        }else{
            alert("Data: " + data + "\nStatus: " + status);
        }
    });
}

function refreshMeterInfo() {
    $("#meterVoltage").val(MeterData.Voltage)
    $("#meterCurrent").val(MeterData.Current)
    $("#meterPower").val(MeterData.Power)
    $("#meterApparentPower").val(MeterData.ApparentPower)
    $("#meterPowerFactor").val(MeterData.PowerFactor)
    $("#meterElectricity").val(MeterData.Electricity)
    $("#meterRunTime").val(MeterData.RunTime)

    $("#underVoltage").val(MeterData.UnderVoltage)
    $('#underVoltageEnable').prop('checked', MeterData.UnderVoltageEnable);
    $("#overVoltage").val(MeterData.OverVoltage)
    $('#overVoltageEnable').prop('checked', MeterData.OverVoltageEnable);
    $("#overCurrent").val(MeterData.OverCurrent)
    $('#overCurrentEnable').prop('checked', MeterData.OverCurrentEnable);
    $("#overPower").val(MeterData.OverPower)
    $('#overPowerEnable').prop('checked', MeterData.OverPowerEnable);
    $("#underPower").val(MeterData.UnderPower)
    $('#underPowerEnable').prop('checked', MeterData.UnderPowerEnable);
}

function chooseBinClick(){
	$('#file').click();
}

function rebootClick(){
	$('#rebootModal').modal("show")
}

function meterClearClick(){
    $('#meterClearModal').modal("show")
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

function meterClear(){
    $.ajax({
        type: "POST",
        url: "/meter",
        contentType: "application/json",
        dataType: "json",
        data: '{"Electricity":"0","RunTime":"0"}',
        success: function (data) {
            MeterData = data;
            refreshMeterInfo();
        }
    });
    $('#meterClearModal').modal("toggle")
}

function meterRefreshChange(){
    clearInterval(meterRefreshTimer);
    if( $("#meterRefreshSelect").val() != 0 ){
        meterRefreshTimer = setInterval(getMeterInfo, $("#meterRefreshSelect").val()*1000);
	}
    setWebSet();
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
			$("#timeName").val(data[0].Name);
            $('#timeEnable').attr('checked', data[0].Enable);
			$("#onTime").val(data[0].OnTime);
			$("#onTimeEnable").attr('checked', data[0].OnEnable);
			$("#offTimeEnable").attr('checked', data[0].OffEnable);
			$("#offTime").val(data[0].OffTime);
			$("#timerCascodeNum").val(data[0].CascodeNum);
			$("#timerCascodeEnable").attr('checked',data[0].Cascode);
			var week= parseInt(data[0].Week);
			for ( var i = 0; i < 7; i++ ){
				switch(i){
					case 0: $("#week1").attr('checked',week&(1<<i));break;
					case 1: $("#week2").attr('checked',week&(1<<i));break;
					case 2: $("#week3").attr('checked',week&(1<<i));break;
					case 3: $("#week4").attr('checked',week&(1<<i));break;
					case 4: $("#week5").attr('checked',week&(1<<i));break;
					case 5: $("#week6").attr('checked',week&(1<<i));break;
					case 6: $("#week7").attr('checked',week&(1<<i));break;
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
			$("#delayName").val(data[0].Name);
			$("#delayEnable").attr('checked', data[0].Enable);
			$("#onInterval").val(data[0].OnInterval);
			$("#onIntervalEnable").attr('checked', data[0].OnEnable);
			$("#offIntervalEnable").attr('checked', data[0].OffEnable);
			$("#offInterval").val(data[0].OffInterval);
			$("#delayCascodeNum").val(data[0].CascodeNum);
			$("#delayCascodeEnable").attr('checked', data[0].Cascode);
			$("#cycleTimes").val(data[0].CycleTimes);
		}else{
			alert("Data: " + data + "\nStatus: " + status);
		}});

	$('#delaySubmitModal').modal("show")
}

function tabInfraredSubmit(){
    numStr = $(this).parents("tr").find('td').eq(0).text();
    url="infrared/"+numStr;
    $.get(url, function(data, status){
        if (status == "success"){
            $('#infraredModalHead').text("红外控制 " + data[0].Num);
            $("#infraredName").val(data[0].Name);
            $("#infraredEnable").attr('checked', data[0].Enable);
            $("#infraredOnValue").val(data[0].OnValue);
            $("#infraredOffValue").val(data[0].OffValue);
        }else{
            alert("Data: " + data + "\nStatus: " + status);
        }});

    $('#infraredSubmitModal').modal("show")
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
	data.PlugName=$("#plugName").val();
	if (data.PlugName.length > 32 ){
		ShowInfo("名称超过长度限制");
		return;
	}
    data.RelayPowerUp=parseInt($("#relayPowerUp").val());

	$("#devName").text("智能插座 （" + data.PlugName + "）");
	$("#title").text(data.PlugName);
	if ( $("#modeSelect").val() == 2 ){
		data.WifiMode= 2;
	}else if ( $("#modeSelect").val() == 1 ){
		data.WifiMode= 1;

        if (SysData.WifiMode == 2) {
            data.WifiSSID=$("#wifiCustom").val();
        }else{
            data.WifiSSID=$("#wifiList option:selected").text();
        }

		data.WifiPasswd=$("#wifiPasswd").val();
		data.SmartConfigFlag=true;
	}else if ( $("#modeSelect").val() == 3 ){
		data.WifiMode = 1;
		data.SmartConfigFlag = false;
	}

	if ( data.WifiMode == 1 ){
        data.CloudPlatform = parseInt($("#PlatformSelect").val());
        if ( data.CloudPlatform == 1 ){
            data.MqttProductKey = $("#ProductKey").val();
            data.MqttDevName = $("#DeviceName").val();
            data.MqttDevSecret = $("#DeviceSecret").val();
        }else if ( data.CloudPlatform == 2 ){
            data.BigiotDevId = $("#BigiotDeviceId").val();
            data.BigiotApiKey = $("#BigiotApiKey").val();
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


function cloudPlatformCommitClick(){
    var data = {};

    if ( $("#PlatformSelect").val() == "0" ){

        data.CloudPlatform = 0;
    }else if ( $("#PlatformSelect").val() == "1" ){

        data.CloudPlatform = 1;
        data.MqttProductKey = $("#ProductKey").val();
        data.MqttDevName = $("#DeviceName").val();
        data.MqttDevSecret = $("#DeviceSecret").val();

    }else if ( $("#PlatformSelect").val() == "2" ){

        data.CloudPlatform = 2;
        data.DevType = parseInt($("#BigiotDeviceTypeSelect option:selected").val());
        data.BigiotDevId = $("#BigiotDeviceId").val();
        data.BigiotApiKey = $("#BigiotApiKey").val();
        data.SwitchId = $("#BigiotIfSwitchStatus").val();
        data.TempId = $("#BigiotIfTemp").val();
        data.HumidityId = $("#BigiotIfHumidity").val();

        data.VoltageId = $("#BigiotIfVoltage").val();
        data.CurrentId = $("#BigiotIfCurrent").val();
        data.PowerId = $("#BigiotIfPower").val();
        data.ElectricityId = $("#BigiotIfElectricity").val();
    }

    $.ajax({
        type: "POST",
        url: "/cloudplatform",
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
	$("#wifiPasswd").val("");
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
            var hour = parseInt( DelayRefreshTime.split(":")[0]);
            var min = parseInt( DelayRefreshTime.split(":")[1]);
            var timePoint = hour * 60 + min;
            var today = new Date(dateStr);
            var nowTimePoint = today.getHours() * 60 + today.getMinutes();
            var time = timePoint - nowTimePoint;
            if (time < 0 ) {
                time = time + 24 * 60;
            }
            hour = parseInt(time/60);
            min = time % 60;

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
            $("#delayTask").text("提示："+item.Name+" 将在 "+hour.toString()+ " 小时"+min.toString()+"分钟后" + action);
            return false;
        }
        index++;
    });
    if(index >= data.length){
        $("#delayTaskClass").addClass("hidden");
        $("#delayTask").text("提示：无延时任务");
        DelayRefreshTime = "";
    }
}


function infraredOnClick(){
    $("#infraredOnValue").val("0");
    $("#infraredOnBtn").text("正在学习");
    var numStr = $("#infraredModalHead").text().split(" ")[1];
	var url = "/infrared/" + numStr + "/switch/on"
    $.get( url, function(data, status){
        if (status == "success"){
            $("#infraredOnValue").val(data.Value);
            $("#infraredOnBtn").text("重新学习");
        }else{
            ShowInfo("Data: " + data + "\nStatus: " + status);
        }
    });
}

function infraredOffClick(){
    $("#infraredOffValue").val("0");
    $("#infraredOffBtn").text("正在学习");
    var numStr = $("#infraredModalHead").text().split(" ")[1];
    var url = "/infrared/" + numStr + "/switch/off"
    $.get( url, function(data, status){
        if (status == "success"){
            $("#infraredOffValue").val(data.Value);
            $("#infraredOffBtn").text("重新学习");

        }else{
            ShowInfo("Data: " + data + "\nStatus: " + status);
        }
    });
}


function meterSubmit(){
    var data = {};
    data.UnderVoltage=$("#underVoltage").val();
    data.OverVoltage=$("#overVoltage").val();
    data.OverCurrent=$("#overCurrent").val();
    data.OverPower=$("#overPower").val();
    data.UnderPower=$("#underPower").val();

    data.UnderVoltageEnable=$("#underVoltageEnable").is(':checked');
    data.OverVoltageEnable=$("#overVoltageEnable").is(':checked');
    data.OverCurrentEnable=$("#overCurrentEnable").is(':checked');
    data.OverPowerEnable=$("#overPowerEnable").is(':checked');
    data.UnderPowerEnable=$("#underPowerEnable").is(':checked');

    $.ajax({
        type: "POST",
        url: "/meter",
        contentType: "application/json",
        dataType: "json",
        data: JSON.stringify(data),
        success: function (data) {
            MeterData = data;
            refreshMeterInfo();
            ShowInfo("设置成功");
        },
        error: function (jqXHR, textStatus, errorThrown) {
            ShowInfo(jqXHR.responseText);
        }
    });
}
