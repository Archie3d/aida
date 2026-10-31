package GenericSynchronizedModel is
    type Base is limited interface;
    procedure Ping (X : Base) is null;
    type Extra is synchronized interface;
    function Read (X : Extra) return Integer is abstract;
end GenericSynchronizedModel;
