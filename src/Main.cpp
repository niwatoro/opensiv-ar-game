// Command + Shift + Bでビルド

#include <opencv2/opencv.hpp>
#include <iostream>
#include <Siv3D.hpp>

// アイテム情報を格納する構造体
struct ItemInfo
{
  Texture texture;
  double speed;
  int32 score;
};

// アイテムの現在の状態を格納する構造体
struct Item
{
  int32 type;
  Vec2 pos;
};

void Main()
{
  Window::Resize(1280, 720);

  // ウェブカメラを非同期で初期化
  AsyncTask<Webcam> task{
      []()
      { return Webcam{0, Size{1280, 720}, StartImmediately::Yes}; }};

  Webcam webcam;
  Image image;
  DynamicTexture texture;

  QRScanner qrScanner;
  Array<QRContent> contents;

  const Font font{FontMethod::MSDF, 48, Typeface::Bold};

  // アイテム生成と描画に関する定数
  constexpr double ItemSpawnInterval = 0.5;
  constexpr double ItemRadius = 80.0;

  // アイテム情報の配列
  const Array<ItemInfo> ItemInfos = {
      {Texture{U"example/utas-info2.gif"}, 100.0, 100},
      {Texture{U"example/school_text_tani.png"}, 1000.0, 1000},
  };

  double itemSpawnAccumulatedTime = 0;
  Vec2 playerPos = Scene::Center();
  Array<Item> items;
  int32 score = 0;

  // 効果音の読み込み
  const Audio audio{Audio::Stream, U"example/shot.mp3"};
  const Audio goukakuAudio{Audio::Stream, U"example/goukaku.mp3"};

  while (System::Update())
  {
    // macOSの場合、ウェブカメラの再試行ボタンを表示
#if SIV3D_PLATFORM(MACOS)
    if (not webcam)
    {
      if (SimpleGUI::Button(U"Retry", Vec2{20, 20}))
      {
        task = AsyncTask{
            []()
            {
              return Webcam{0, Size{1280, 720}, StartImmediately::Yes};
            }};
      }
    }
#endif

    // ウェブカメラの初期化が完了したら取得
    if (task.isReady())
    {
      webcam = task.get();
    }

    // 新しいフレームがあれば取得してQRコードをスキャン
    if (webcam.hasNewFrame())
    {
      webcam.getFrame(image);
      texture.fill(image);
      contents = qrScanner.scan(image);
    }

    // ウェブカメラが使用できない場合はローディングアニメーションを表示
    if (not webcam)
    {
      Circle{Scene::Center(), 40}.drawArc(Scene::Time() * 180_deg, 300_deg, 5, 5);
    }

    if (texture)
    {
      const double deltaTime = Scene::DeltaTime();

      // アイテムの生成と更新
      {
        itemSpawnAccumulatedTime += deltaTime;

        // 一定間隔でアイテムを生成
        while (itemSpawnAccumulatedTime >= ItemSpawnInterval)
        {
          items << Item{
              .type = (RandomBool(0.75) ? 0 : 1),
              .pos = Vec2{Random(40, 1240), -ItemRadius}};

          itemSpawnAccumulatedTime -= ItemSpawnInterval;
        }

        // アイテムの位置を更新
        for (auto &item : items)
        {
          item.pos.y += ItemInfos[item.type].speed * deltaTime;
        }

        // QRコードとアイテムの衝突判定
        for (auto it = items.begin(); it != items.end();)
        {
          const Circle itemCircle{it->pos, ItemRadius};

          bool erased = false;
          for (auto &content : contents)
          {
            const bool intersects = content.quad.intersects(itemCircle);
            if (intersects)
            {
              // 衝突時の効果音再生とスコア加算
              if (it.base()->type == 1)
              {
                goukakuAudio.play();
              }
              else
              {
                audio.play();
              }

              score += ItemInfos[it->type].score;
              it = items.erase(it);
              erased = true;
              break;
            }
          }

          if (!erased)
          {
            ++it;
          }
        }

        // 画面外に出たアイテムを削除
        items.remove_if([](const Item &item)
                        { return (item.pos.y > 720 + ItemRadius); });
      }

      // 描画処理
      {
        texture.mirrored().draw();

        // アイテムの描画
        for (const auto &item : items)
        {
          ItemInfos[item.type].texture.resized(ItemRadius * 2).drawAt(Vec2{1280 - item.pos.x, item.pos.y});
        }

        // QRコードの枠を描画
        for (const auto &content : contents)
        {
          const auto quad = content.quad;
          Quad{
              Vec2{1280 - quad.p0.x, quad.p0.y},
              Vec2{1280 - quad.p1.x, quad.p1.y},
              Vec2{1280 - quad.p2.x, quad.p2.y},
              Vec2{1280 - quad.p3.x, quad.p3.y},
          }
              .drawFrame(4, Palette::Red);
        }

        // スコアの表示
        font(ThousandsSeparate(score)).draw(TextStyle::Outline(0.2, ColorF{0, 0, 0}), 30, Vec2{20, 20});
      }
    }
  }
}