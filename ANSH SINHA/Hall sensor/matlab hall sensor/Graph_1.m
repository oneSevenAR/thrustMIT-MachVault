
% This script connects to an Arduino, reads serial data, and plots it in real-time.

% 1. Clear previous data and plots
clear all;
close all;

% 2. User-defined settings
% IMPORTANT: Change the 'COM_PORT' to match your Arduino's serial port.
% You can find this in the Arduino IDE under 'Tools' -> 'Port'.
COM_PORT = 'COM5'; % Example: 'COM3' on Windows, '/dev/ttyACM0' on Linux, '/dev/cu.usbmodem1411' on Mac.
BAUD_RATE = 9600; % Must match the baud rate in your Arduino code.

% 3. Create and open the serial port object
try
    % Check if the serial port is already open and close it if necessary
    if ~isempty(instrfind)
        fclose(instrfind);
        delete(instrfind);
    end

    % Create the serial port object
    s = serialport(COM_PORT, BAUD_RATE);
    
    % Configure the terminator to read a full line of data
    configureTerminator(s,"LF");
    
    fprintf('Successfully connected to Arduino on port %s\n', COM_PORT);
    pause(1); % Wait a moment for the connection to stabilize

    % 4. Set up the live plot
    figure('Name','Arduino Live Plot');
    title('Real-time Analog Read from Arduino');
    xlabel('Time (samples)');
    ylabel('Analog Value (0-1023)');
    grid on;
    
    % Use animatedline for a fast, efficient live plot
    h = animatedline('Color', 'b', 'LineWidth', 1.5);
    ax = gca;
    ax.YLim = [0 1024];
    
    fprintf('Starting data acquisition and plotting...\n');
    
    dataPoints = 0; % Counter for the x-axis
    tic; % Start a timer
    
    % 5. Main loop for reading and plotting data
    while true
        % Check if there is data available to read
        if s.NumBytesAvailable > 0
            % Read a line of text from the serial buffer
            dataString = readline(s);
            
            % Convert the string to a number
            analogValue = str2double(dataString);
            
            % Check if the conversion was successful
            if ~isnan(analogValue)
                dataPoints = dataPoints + 1;
                addpoints(h, dataPoints, analogValue);
                drawnow limitrate; % Update the plot
            end
        end
    end

catch ME
    % 6. Clean up if an error occurs (e.g., user stops the script)
    fprintf('\nAn error occurred: %s\n', ME.message);
    fprintf('Closing serial port and cleaning up...\n');
    
    % Make sure the serial port is closed and deleted to free up the port
    if exist('s', 'var') && isvalid(s)
        fclose(s);
        delete(s);
    end
    
    % Find and delete all remaining serial port objects
    delete(instrfindall);
    
    fprintf('Cleanup complete.\n');
end
