% Calcuclates the pressure difference over the last 3 hours
% writes to channel as long as the time difference between 
% the two points is at least 2 hours

% Channel ID to read data from
ChannelID = 101982;
% Pressure Field IDs
PressureFieldID = 2;
PressureChangeID = 3;

% TODO - Put your API keys here:
writeAPIKey = 'your write API key';
readAPIKey = 'your read API key'; % this is only necessary if the channel is private

% Get humidity data for the last 60 minutes from the MathWorks Weather
% Station Channel. Learn more about the THINGSPEAKREAD function by going to
% the Documentation tab on the right side pane of this page.

[pressure, timestamps, chInfo] = thingSpeakRead(ChannelID, 'ReadKey', readAPIKey,  'Fields', PressureFieldID, 'NumMinutes', 180);

[m,n]=size(pressure)
display(m)

if m > 1 % we need at least 2 points to do the calculation
	% Calculate the pressure change
	pressureChange = pressure(end)-pressure([1]);
	% pressure(end) gets us the most recent pressure reading, which is last in the pressure variable (a Matlab matrix) 
	display(pressureChange, 'pressure change (inHg)'); % this shows up below when you hit run
	timeDiff = minutes(diff([timestamps([1]), timestamps(end)])); % difference in minutes between the first and last readings
	display(timeDiff);
	pressureChangeRate = pressureChange/timeDiff * 1000000;
	display(pressureChangeRate, 'pressure change rate (inHg/min)*10^6');
	
	
	% Write the average humidity to another channel specified by the
	% 'writeChannelID' variable
	
	% Learn more about the THINGSPEAKWRITE function by going to the Documentation tab on
	% the right side pane of this page.
	if timeDiff > 60.0 % make sure the time difference is at least 1 hour between the points
		for n=1:8 % quite a bit of a hack, but it works
			pause(2); % they don't allow more than 2s pauses (delays) here, and I didn't take the time
			% to figure out how to do a callback, etc
		end
		thingSpeakWrite(ChannelID, pressureChangeRate, 'Fields', PressureChangeID, 'writekey', writeAPIKey);
		display('writing to channel')
	end
else
	display('not enough data in channel yet')
end