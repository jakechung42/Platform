function data = sCollectSineData(s)
%this function collect the sinusoidal data from the Arduino for the
%Platform project

%open to collect. If program is terminated prematurely, do fclose
fopen(s);
%make the cell to collect the data
%this number need to match with the data size of the Arduino
length_to_collect = 5000;

%cell to store the data
collect = cell(length_to_collect, 1);
%remove the first 20 lines of the serial port 
fprintf('Begin rejecting...\n')
for i = 1:20
    temp = fgetl(s);
end
fprintf('Reject complete, start collecting\n')
%collect data continuously
for i = 1:length_to_collect
    collect{i} = fgetl(s);
end
disp('Collection is completed.')
%close the port
fclose(s);
data = collect;
% %convert data to num array
% data = zeros(length_to_collect,2);
% for i = 1:length_to_collect
%     data(i, :) = str2num(collect{i});
% end

end