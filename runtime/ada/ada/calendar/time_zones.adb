package body Ada.Calendar.Time_Zones is
    function UTC_Time_Offset (Date : Time := Clock) return Time_Offset is
    begin
        -- Calendar's implementation-defined zone is UTC on every host.
        return 0;
    end UTC_Time_Offset;
end Ada.Calendar.Time_Zones;
