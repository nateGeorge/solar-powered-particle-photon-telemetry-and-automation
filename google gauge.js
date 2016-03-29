<script type='text/javascript' src='https://ajax.googleapis.com/ajax/libs/jquery/1.9.1/jquery.min.js'></script>
<script type='text/javascript' src='https://www.google.com/jsapi'></script>
<script type='text/javascript'>

  // set your channel id here
  var channel_id = channel ID here; // eg 101992
  // set your channel's read api key here
  var api_key = 'your readAPI key here';
  // maximum value for the gauge
  var max_gauge_value = 42; // this is the maximum water height from your telemetry code
  // name of the gauge
  var gauge_name = 'Water tank level (%)';

  // global variables
  var chart, charts, data;

  // load the google gauge visualization
  google.load('visualization', '1', {packages:['gauge']});
  google.setOnLoadCallback(initChart);

  // display the data
  function displayData(point) {
	data.setValue(0, 0, gauge_name);
	data.setValue(0, 1, point);
	chart.draw(data, options);
  }

  // load the data
  function loadData() {
	// variable for the data point
	var p;

	// get the data from thingspeak
	$.getJSON('https://api.thingspeak.com/channels/' + channel_id + '/feed/last.json?api_key=' + api_key, function(data) {

	  // get the data point
	  p = data.field1;

	  // if there is a data point display it
	  if (p) {
		p = Math.round((p / max_gauge_value) * 100);
		displayData(p);
	  }

	});
  }

  // initialize the chart
  function initChart() {

	data = new google.visualization.DataTable();
	data.addColumn('string', 'Label');
	data.addColumn('number', 'Value');
	data.addRows(1);

	chart = new google.visualization.Gauge(document.getElementById('gauge_div'));
	options = {width: 200, height: 200, redFrom: 0, redTo: 20, yellowFrom:20, yellowTo: 50, greenFrom: 50, greenTo: 100, minorTicks: 5}; // customize the red, yellow, and green levels if you want

	loadData();

	// load new data every 15 seconds
	setInterval('loadData()', 15000);
  }

</script>