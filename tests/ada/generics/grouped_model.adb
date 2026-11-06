package body Grouped_Model is
    function Equal return Boolean is
    begin
        return Agree;
    end Equal;
    procedure Update is
    begin
        X := X + Step;
        Y := Y + Extra;
    end Update;
end Grouped_Model;
