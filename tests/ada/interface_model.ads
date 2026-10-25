package Interface_Model is
    type Reader is interface;
    function Read (Self : Reader) return Integer is abstract;
    type Meter is interface;
    function Measure (Self : Meter) return Integer is abstract;
    procedure Exercise (Self : in out Reader'Class);
private
    procedure Secret (Self : in out Reader) is null;
end Interface_Model;
