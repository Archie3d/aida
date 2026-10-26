package GenericInterfaceModel is
    type Base is interface;
    function Read (X : Base) return Integer is abstract;
    type Extra is interface;
    procedure Ping (X : Extra) is null;
    type Both is interface and Extra and Base;
    type Item is new Both with record
        Value : Integer := 43;
    end record;
    overriding function Read (X : Item) return Integer;
end GenericInterfaceModel;
