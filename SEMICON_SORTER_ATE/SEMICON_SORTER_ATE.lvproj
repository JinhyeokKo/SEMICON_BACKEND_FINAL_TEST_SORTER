<?xml version='1.0' encoding='UTF-8'?>
<Project Type="Project" LVVersion="26008000">
	<Property Name="NI.LV.All.SaveVersion" Type="Str">26.0</Property>
	<Property Name="NI.LV.All.SourceOnly" Type="Bool">true</Property>
	<Item Name="My Computer" Type="My Computer">
		<Property Name="server.app.propertiesEnabled" Type="Bool">true</Property>
		<Property Name="server.control.propertiesEnabled" Type="Bool">true</Property>
		<Property Name="server.tcp.enabled" Type="Bool">false</Property>
		<Property Name="server.tcp.port" Type="Int">0</Property>
		<Property Name="server.tcp.serviceName" Type="Str">My Computer/VI Server</Property>
		<Property Name="server.tcp.serviceName.default" Type="Str">My Computer/VI Server</Property>
		<Property Name="server.vi.callsEnabled" Type="Bool">true</Property>
		<Property Name="server.vi.propertiesEnabled" Type="Bool">true</Property>
		<Property Name="specify.custom.address" Type="Bool">false</Property>
		<Item Name="AppendInspectionRow.vi" Type="VI" URL="../AppendInspectionRow.vi"/>
		<Item Name="ATE_CommCheck.vi" Type="VI" URL="../ATE_CommCheck.vi"/>
		<Item Name="ATE_ManualReply.vi" Type="VI" URL="../ATE_ManualReply.vi"/>
		<Item Name="JudgeVoltage.vi" Type="VI" URL="../JudgeVoltage.vi"/>
		<Item Name="VirtualATE.vi" Type="VI" URL="../VirtualATE.vi"/>
		<Item Name="VirtualATE_V2.vi" Type="VI" URL="../VirtualATE_V2.vi"/>
		<Item Name="VirtualATE_V2_after_step8.vi" Type="VI" URL="../VirtualATE_V2_after_step8.vi"/>
		<Item Name="VirtualATE_V3.vi" Type="VI" URL="../VirtualATE_V3.vi"/>
		<Item Name="Dependencies" Type="Dependencies"/>
		<Item Name="Build Specifications" Type="Build"/>
	</Item>
</Project>
