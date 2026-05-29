
from pymongo import MongoClient
from enum import Enum


class EdgeOrchestration(Enum):
    ACCESS_ON = 0
    FIX_ON = 1
    ALL_ON = 2
    ALL_OFF = 3
    FUZZY = 4
    REMOTE = 5
    RL = 6
    RANDOM = 7


class EdgeControl(Enum):
    OFF = 0
    FUZZY = 1
    Q_LEARNING = 2


class SatDataBase:
    onos_url = "http://localhost:8181"
    onos_auth = ("onos", "rocks")

    def __init__(self, db_url='localhost'):
        try:
            self.db = self.get_database(db_url=db_url)
            # Test connection
            self.db.command('ping')
            self.enabled = True
        except Exception as e:
            print(f"WARNING: Could not connect to MongoDB at {db_url}. Database operations will be mocked.")
            self.enabled = False
        
        self.onos_url = f'http://{db_url}:8181'
        self.collections = ['tasks', 'servers', 'nodes',
                            'net_measures', 'sim_status', 'servers_hist', 'connections_hist']

    def drop_db(self):
        if not self.enabled: return
        for col in self.collections:
            self.drop_collection(col)

    def upsert_in_collection(self, item, collection='tasks', id='_id'):
        if not self.enabled: return
        try:
            self.db[collection].update_one({id: item[id]}, {"$set": item}, upsert=True)
        except Exception as e:
            print('Exception happend updating {}: {}'.format(item, e))


    def insert_in_collection(self, item, collection='tasks'):
        if not self.enabled: return
        self.db[collection].insert_one(item)

    def drop_collection(self, collection='tasks'):
        if not self.enabled: return
        self.db[collection].drop()

    def get_items_in_collection(self, query=None, collection='tasks', sort=None, limit=None):
        if not self.enabled: return []
        items = self.db[collection].find(query) if sort is None else\
            self.db[collection].find(query).sort(*sort)
        if limit is not None:
            items = items.limit(limit)
        return [item for item in items]

    def get_item_in_collection(self, query=None, collection='tasks'):
        if not self.enabled: return None
        return self.db[collection].find_one(query)

    def get_database(self, db_name='sat_net', db_url='localhost'):
        # Provide the mongodb atlas url to connect python to mongodb using pymongo
        CONNECTION_STRING = "mongodb://{}:27017/SatNet".format(db_url)
        # Create a connection using MongoClient.
        client = MongoClient(CONNECTION_STRING)
        # Create the database
        return client['{}'.format(db_name)]