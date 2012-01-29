#include <Box2D/Box2D.h>
#include <SFML/Graphics.hpp>
#include <iostream>

class World {
public:
  World(const float _gravX, const float _gravY);
  ~World();

  void stepIteration() const;

  b2World& getWorldHandle();

  float mTimeStep;
  int velocityIterations, positionIterations;

private:
  b2World* mWorld;
};

World::World(const float _gravX, const float _gravY) {
  mWorld = new b2World(b2Vec2(_gravX, _gravY));
}

World::~World() {
  delete mWorld;
}

b2World& World::getWorldHandle() { return *mWorld; }

void World::stepIteration() const {
  mWorld->Step(mTimeStep, velocityIterations, positionIterations);
}

class Actor {
public:
  Actor(World* _world, float _x, float _y);
  ~Actor();

  void startContact();
  void endContact();

  void Render(sf::RenderWindow& _window);

  sf::RectangleShape mShape;
  b2BodyDef mBodyDef;
  b2Body* mMyBody;
  b2PolygonShape dynamicBox;
  b2FixtureDef fixtureDef;
  bool isContacting;
};

Actor::Actor(World* _world, float _x, float _y) {
  mShape = sf::RectangleShape(sf::Vector2f(20, 20));
  mShape.SetPosition(_x, _y);
  mShape.SetOutlineColor(sf::Color::Red);
  mShape.SetFillColor(sf::Color::Transparent);
  mShape.SetOutlineThickness(1);
  mShape.SetOrigin(mShape.GetSize().x/2, mShape.GetSize().y/2);

  mBodyDef.type = b2_dynamicBody;
  mBodyDef.position.Set(_x, _y);
  mBodyDef.active = true;

  mMyBody = _world->getWorldHandle().CreateBody(&mBodyDef);
  // For now, make the userData a body pointer for easy reference for collision stuff
  // Later could do something with if its a gameobject or entity or something
  mMyBody->SetUserData(this);

  
  dynamicBox.SetAsBox(mShape.GetSize().x/2, mShape.GetSize().y/2);

  fixtureDef.shape = &dynamicBox;
  fixtureDef.density = 0.1f;
  fixtureDef.friction = 0.1f;

  mMyBody->CreateFixture(&fixtureDef);

  isContacting = false;
}

Actor::~Actor() {
}

void Actor::startContact() { isContacting = true; }
void Actor::endContact() { isContacting = false; }

void Actor::Render(sf::RenderWindow& _window) {
  if (isContacting)
    mShape.SetFillColor(sf::Color::Yellow);
  else
    mShape.SetFillColor(sf::Color::Red);
  _window.Draw(mShape);
}

class CustomContactListener : public b2ContactListener {
  void BeginContact(b2Contact* contact) {
    // if fixture A is an Actor
     void* bodyUserData = contact->GetFixtureA()->GetBody()->GetUserData();
    if (bodyUserData)
      static_cast<Actor*>(bodyUserData)->startContact();

    // if fixture B is an Actor
    bodyUserData = contact->GetFixtureB()->GetBody()->GetUserData();
    if (bodyUserData)
      static_cast<Actor*>(bodyUserData)->startContact();
  }

  void EndContact(b2Contact* contact) {
    // if fixture A is an Actor
    void* bodyUserData = contact->GetFixtureA()->GetBody()->GetUserData();
    if (bodyUserData)
      static_cast<Actor*>(bodyUserData)->endContact();

    // if fixture B is an Actor
    bodyUserData = contact->GetFixtureB()->GetBody()->GetUserData();
    if (bodyUserData)
      static_cast<Actor*>(bodyUserData)->endContact();
  }
};


class Level {
public:
  Level();
  ~Level();

private:
};

Level::~Level() {}

int main() {
  // testing out chain shapes
  b2Vec2 verts[3];
  verts[0].Set(600, 300);
  verts[1].Set(630, 300);
  verts[2].Set(615, 307);
  bool running = true;
  sf::RenderWindow window(sf::VideoMode(800, 600), "Box2d + SFML");
  CustomContactListener myContactListenerInstance;

  World myWorld(0, 9.8f); // Errf gravi'eh
  myWorld.mTimeStep = 1.f/60.f; myWorld.positionIterations = 2; myWorld.velocityIterations = 6;

  myWorld.getWorldHandle().SetContactListener(&myContactListenerInstance);

  Actor player(&myWorld, 400, 50);

  // Chain Shape
  b2BodyDef chainBodyDef;
  chainBodyDef.type = b2_staticBody;
  chainBodyDef.position.Set(verts[0].x, verts[0].y);
  chainBodyDef.active = true;

  b2EdgeShape chain;
  chain.Set(verts[0], verts[1]);

  b2Body* chainBody = myWorld.getWorldHandle().CreateBody(&chainBodyDef);

  chainBody->CreateFixture(&chain, 1.f);

  sf::RectangleShape chainShape;
  chainShape.SetPosition(600, 300);
  chainShape.SetFillColor(sf::Color::Green);
  chainShape.SetSize(sf::Vector2f(30,1));

  // /Chain Shape


  b2BodyDef groundBodyDef;
  groundBodyDef.type = b2_staticBody;
  groundBodyDef.position.Set(300, 300);
  groundBodyDef.active = true;

  b2Body* groundBody = myWorld.getWorldHandle().CreateBody(&groundBodyDef);


  //Ground Rectangle
  sf::RectangleShape groundShape(sf::Vector2f(300, 100));
  groundShape.SetPosition(groundBodyDef.position.x, groundBodyDef.position.y);
  groundShape.SetFillColor(sf::Color::White);
  groundShape.SetOutlineThickness(0);
  groundShape.SetOrigin(groundShape.GetSize().x/2, groundShape.GetSize().y/2);
  // /rect
  
  b2PolygonShape groundBox;
  groundBox.SetAsBox(groundShape.GetSize().x/2, groundShape.GetSize().y/2);

  groundBody->CreateFixture(&groundBox, 1.f);

  while (window.IsOpen()) {
    b2Vec2 vel = player.mMyBody->GetLinearVelocity();
    float desiredVel = 0, velChange = 0, impulse = 0;;

    sf::Event myEvent;
    while (window.PollEvent(myEvent)) {
      if (myEvent.Type == sf::Event::KeyReleased) {
        if (myEvent.Key.Code == sf::Keyboard::Escape || myEvent.Type == sf::Event::Closed)
          window.Close();

        // Jump
        if (myEvent.Key.Code == sf::Keyboard::Space) {
          desiredVel = -20;
          velChange = desiredVel - vel.y;
          impulse = player.mMyBody->GetMass() * velChange;
          player.mMyBody->ApplyLinearImpulse(b2Vec2(0, impulse), player.mMyBody->GetWorldCenter());
        }
      }
    }

    if (sf::Keyboard::IsKeyPressed(sf::Keyboard::Left)) {
      desiredVel = -5;
      velChange = desiredVel - vel.x;
      impulse = player.mMyBody->GetMass() * velChange;
      player.mMyBody->ApplyLinearImpulse(b2Vec2(impulse, 0),  player.mMyBody->GetWorldCenter());

    } else if (sf::Keyboard::IsKeyPressed(sf::Keyboard::Right)) {
      desiredVel = 5;
      velChange = desiredVel - vel.x;
      impulse = player.mMyBody->GetMass() * velChange;
      player.mMyBody->ApplyLinearImpulse(b2Vec2(impulse, 0),  player.mMyBody->GetWorldCenter());
    }

    player.mShape.SetPosition(player.mMyBody->GetPosition().x, player.mMyBody->GetPosition().y);

    //std::cout << "(Player) x: " << player.mMyBody->GetPosition().x << " y: " << player.mMyBody->GetPosition().y << std::endl;
    //std::cout << "(Ground) x: " << groundBody->GetPosition().x << " y: " << groundBody->GetPosition().y << std::endl;
      
    window.Clear();
    // put shit here
      
    window.Draw(groundShape);
    window.Draw(chainShape);
    player.Render(window);

    // /putshithere
    myWorld.stepIteration();

    window.Display();
  }

  myWorld.getWorldHandle().DestroyBody(groundBody);
  myWorld.getWorldHandle().DestroyBody(player.mMyBody);

  return EXIT_SUCCESS;
}